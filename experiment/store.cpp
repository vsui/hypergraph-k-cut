//
// Created by Victor on 2/23/20.
//

#include "store.hpp"

#include <iostream>
#include <fstream>

FilesystemStore::FilesystemStore(std::filesystem::path path)
    : root_path_(path), cuts_path_(path / "cuts"), runs_path_(path / "runs"), hgrs_path_(path / "hypergraphs") {}

/*
 * This stores all data in a directory at `path_`.
 *
 * The directory has this structure:
 *
 * root/
 * - cuts/
 *   - <name>/
 *     - <ID>.2cut
 *   - <name2>/
 *     - <ID>.3cut
 * - runs/
 *   - name.2cut.runs
 * - hypergraphs/
 *   - name.hgr
 *
 * A 'hypergraphs/<name>.hgr' file is just a hypergraph in .hMETIS format.
 *
 * A 'cuts/<name>/<ID>.<k>cut' file holds a k-cut for the <name> hypergraph. A hypergraph may have multiple
 * cuts, hence the need for a cut ID.
 *
 * A runs/<name>.2cut.runs holds information about individual runs of a cut algorithm in a CSV format.
 */

namespace fs = std::filesystem;
using std::begin, std::end;

CutIterator::CutIterator(fs::path path) : hgrs_(path), cuts_(hgrs_->path()), end_(false) {}

CutInfo CutIterator::operator*() {
  CutInfo info;
  std::cout << hgrs_->path() / cuts_->path() << std::endl;

  std::ifstream in(cuts_->path(), std::ios_base::in);
  in >> info;
  info.id = std::stoul(cuts_->path().filename());
  return info;
}

CutIterator &CutIterator::operator++() {
  ++cuts_;
  if (cuts_ == fs::end(cuts_)) {
    ++hgrs_;
    if (hgrs_ == fs::end(hgrs_)) {
      end_ = true;
    } else {
      cuts_ = fs::directory_iterator(hgrs_->path());
    }
  }
  return *this;
}

CutIterator::CutIterator() : end_(true) {}

CutIterator CutIterator::begin() { return *this; }

CutIterator CutIterator::end() { return CutIterator(); }

bool CutIterator::operator!=(const CutIterator &it) {
  if (end_) {
    return !it.end_;
  }
  return hgrs_ != it.hgrs_ || cuts_ != it.cuts_;
}

namespace {

/**
 * Return true if path is a directory, attempts to create such a directory otherwise and returns true on success
 * @param path
 * @return
 */
bool probe(const fs::path &path) {
  if (!fs::exists(path)) {
    return fs::create_directory(path);
  }
  return fs::is_directory(path);
}

}

bool FilesystemStore::initialize() {
  return probe(root_path_) && probe(cuts_path_) && probe(runs_path_) && probe(hgrs_path_);
}

ReportStatus FilesystemStore::report(const HypergraphWrapper &hypergraph) {
  // Check if already present
  auto dir_it = fs::directory_iterator(hgrs_path_);
  auto it = std::find_if(begin(dir_it),
                         end(dir_it),
                         [&hypergraph](const fs::directory_entry &entry) {
                           return entry.path().filename().string() == hypergraph.name;
                         });
  if (it != end(dir_it)) {
    return ReportStatus::ALREADY_THERE;
  }

  // Need to create it
  fs::path path(hgrs_path_ / hypergraph.name);
  std::ofstream out(path);

  struct {
    std::ofstream *out;
    void operator()(const Hypergraph &h) { *out << h; }
    void operator()(const WeightedHypergraph<size_t> &h) { *out << h; }
  } visitor;
  visitor.out = &out;

  std::visit(visitor, hypergraph.h);

  return ReportStatus::OK;
}

ReportStatus FilesystemStore::report(const CutInfo &info, uint64_t &id) {
  // Check if already present
  fs::path cuts_dir = cuts_path_ / info.hypergraph;

  if (!probe(cuts_dir)) {
    return ReportStatus::ERROR;
  }

  // Need to check each file for if there is a cut inside it
  auto dir_it = fs::directory_iterator(cuts_dir);

  // See if there is a file where they are equal
  auto it = std::find_if(begin(dir_it), end(dir_it), [info](const fs::directory_entry &entry) {
    std::ifstream in(entry.path());
    CutInfo in_info;
    in >> in_info;
    return in_info == info;
  });

  if (it != end(dir_it)) {
    return ReportStatus::ALREADY_THERE;
  }

  size_t distance = std::distance(begin(dir_it), end(dir_it));
  id = distance;
  std::ofstream out(cuts_dir / std::to_string(distance));
  out << info;

  return ReportStatus::OK;
}

ReportStatus FilesystemStore::report(const CutRunInfo &info) {
  fs::path path = runs_path_ / info.info.hypergraph;

  bool existed = fs::exists(path);

  std::ofstream out(runs_path_ / info.info.hypergraph, std::ios_base::app);

  if (!existed) {
    out << CutRunInfo::csv_header() << std::endl;
  }

  out << info << std::endl;

  return ReportStatus::OK;
}

CutIterator FilesystemStore::cuts() {
  return CutIterator(cuts_path_);
}


