#include "rix/tf/frame_graph.hpp"

#include "rix/rob/eigen_util.hpp"

namespace rix {

FrameGraph::FrameGraph(const std::string &root, const Duration &duration)
    : graph_(1), frames_(1, Frame(root, duration)), duration_(duration) {
    name_to_index_[root] = 0;
}

FrameGraph::FrameGraph(const FrameGraph &other)
    : graph_(other.graph_), frames_(other.frames_), name_to_index_(other.name_to_index_) {}

FrameGraph &FrameGraph::operator=(const FrameGraph &other) {
    if (this != &other) {
        graph_ = other.graph_;
        frames_ = other.frames_;
        name_to_index_ = other.name_to_index_;
        duration_ = other.duration_;
    }
    return *this;
}

bool FrameGraph::exists(const std::string &name) const { return name_to_index_.find(name) != name_to_index_.end(); }

FrameGraph::Iterator FrameGraph::get_root() const { return Iterator(*this, 0); }

/**< TODO: Implement the get_leaves method. */
std::vector<std::string> FrameGraph::get_leaves() const {
    std::vector<std::string> leaves;
    for (size_t i = 1; i < graph_.size(); ++i) {
        if (graph_[i].size() == 1) {
            leaves.push_back(frames_[i].name);
        }
    }
    return leaves;
}

bool FrameGraph::update(const msg::geometry::TF &tf) {
    for (const auto &transform : tf.transforms) {
        if (!update(transform)) return false;
    }
    return true;
}

/**< TODO: Implement the update method. */
bool FrameGraph::update(const msg::geometry::TransformStamped &transform) {
    // If the parent frame does not exist, return false because there is no connection to the graph
    auto parent_it = name_to_index_.find(transform.header.frame_id);
    if (parent_it == name_to_index_.end()) {
        return false;
    }
    int parent_index = parent_it->second;

    // If the child frame does not exist
    auto child_it = name_to_index_.find(transform.child_frame_id);
    int child_index = -1;
    if (child_it == name_to_index_.end()) {
        // If the parent exists and the child does not, insert the child frame into the graph
        int new_index = frames_.size();
        Frame frame(transform.child_frame_id, duration_);
        frames_.push_back(frame);
        graph_.push_back(std::vector<int>(1, parent_index));
        graph_[parent_index].push_back(new_index);
        child_it = name_to_index_.insert({frame.name, new_index}).first;
        child_index = child_it->second;
    } else {
        // If the child frame exists, need to make sure that it is a child of the parent
        child_index = child_it->second;
        auto found_it = std::find(graph_[parent_index].begin(), graph_[parent_index].end(), child_index);
        if (found_it == graph_[parent_index].end()) {
            return false;
        }
    }

    // Update the transform in the child frame
    frames_[child_index].buffer.insert(Time(transform.header.stamp), transform.transform);
    return true;
}

/**< TODO: Implement the get_transform method. */
bool FrameGraph::get_transform(const std::string &target_frame, const std::string &source_frame, Time time,
                               msg::geometry::TransformStamped &transform) const {
    // Assign information to output transform
    transform.header.frame_id = source_frame;
    transform.header.seq = 0;
    transform.header.stamp = time.to_msg();
    transform.child_frame_id = target_frame;

    // Find the target frame in the graph
    auto tgt_it = find(target_frame);
    if (tgt_it == end()) {
        return false;
    }

    // Find the source frame in the graph
    auto src_it = find(source_frame);
    if (src_it == end()) {
        return false;
    }

    // Check if the target and source frames are the same
    if (tgt_it == src_it) {
        transform.transform = transform_identity();
        return true;
    }

    // Find the nearest common ancestor
    FrameGraph::Iterator common_ancestor_it = find_nearest_ancestor(src_it, tgt_it);
    if (common_ancestor_it == end()) {
        return false;
    }

    // Build transform from source to common ancestor
    msg::geometry::Transform t;
    Eigen::Affine3d src_transform = Eigen::Affine3d::Identity();
    while (src_it != common_ancestor_it) {
        if (!src_it->buffer.get(time, t)) {
            return false;
        }
        src_transform = src_transform * msg_to_eigen(t);
        --src_it;
    }

    // Build transform from common ancestor to target
    Eigen::Affine3d tgt_transform = Eigen::Affine3d::Identity();
    while (tgt_it != common_ancestor_it) {
        if (!tgt_it->buffer.get(time, t)) {
            return false;
        }
        tgt_transform = msg_to_eigen(t) * tgt_transform;
        --tgt_it;
    }

    // Chain the transforms together
    transform.transform = eigen_to_msg(src_transform * tgt_transform);

    return true;
}

FrameGraph::Iterator FrameGraph::find(const std::string &name) const {
    auto it = name_to_index_.find(name);
    if (it != name_to_index_.end()) {
        return Iterator(*this, it->second);
    }
    return end();
}

/**< TODO: Implement the find_nearest_ancestor method. */
FrameGraph::Iterator FrameGraph::find_nearest_ancestor(Iterator frame_a, Iterator frame_b) const {
    while (frame_a != frame_b) {
        if (frame_a < frame_b) {
            frame_b--;
        } else {
            frame_a--;
        }
    }
    return frame_a;
}

FrameGraph::Iterator FrameGraph::find_nearest_ancestor(const std::string &frame_a, const std::string &frame_b) const {
    auto it_a = find(frame_a);
    if (it_a == end()) {
        return end();
    }
    auto it_b = find(frame_b);
    if (it_b == end()) {
        return end();
    }
    return find_nearest_ancestor(it_a, it_b);
}

FrameGraph::Iterator::Iterator(const Iterator &other) : graph_(other.graph_), index_(other.index_) {}

FrameGraph::Iterator &FrameGraph::Iterator::operator=(const Iterator &other) {
    if (this != &other) {
        index_ = other.index_;
    }
    return *this;
}

FrameGraph::Iterator FrameGraph::end() const { return Iterator(*this, -1); }

FrameGraph::Iterator::Iterator(const FrameGraph &graph, int index) : graph_(graph), index_(index) {}

FrameGraph::Iterator &FrameGraph::Iterator::operator--() {
    if (index_ > 0) {
        index_ = graph_.graph_[index_][0];
    }
    return *this;
}

FrameGraph::Iterator FrameGraph::Iterator::operator--(int) {
    FrameGraph::Iterator tmp = *this;
    --(*this);
    return tmp;
}

bool FrameGraph::Iterator::operator==(const Iterator &other) const {
    return index_ == other.index_ && &graph_ == &other.graph_;
}
bool FrameGraph::Iterator::operator!=(const Iterator &other) const { return !(*this == other); }
const Frame &FrameGraph::Iterator::operator*() const { return graph_.frames_[index_]; }
const Frame *FrameGraph::Iterator::operator->() const { return &graph_.frames_[index_]; }
bool FrameGraph::Iterator::operator<(const Iterator &other) const { return index_ < other.index_; }
bool FrameGraph::Iterator::operator>(const Iterator &other) const { return index_ > other.index_; }
bool FrameGraph::Iterator::operator<=(const Iterator &other) const { return index_ <= other.index_; }
bool FrameGraph::Iterator::operator>=(const Iterator &other) const { return index_ >= other.index_; }

}  // namespace rix