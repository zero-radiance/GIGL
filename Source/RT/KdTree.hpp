#pragma once

#include "KdTree.h"
#include <algorithm>
#include "..\Common\Timer.h"
#include "..\Common\Utility.hpp"

namespace rt {
    template <class Primitive>
    KdTree<Primitive>::Node::Node(): is_leaf{1}, single_prim_id{0} {
        leaf.n_prims = 0;
    }

    template <class Primitive>
    KdTree<Primitive>::Node::Node(const uint n_prims, const uint* overlap_prim_ids,
                                  std::vector<uint>& leaf_prim_ids) : is_leaf{1} {
        leaf.n_prims = n_prims;
        switch (leaf.n_prims) {
            case 0:							// Empty leaf
                single_prim_id = 0;
                break;
            case 1:							// Store primitive index directly
                single_prim_id = overlap_prim_ids[0];
                break;
            default:						// Two or more primitives
                prims_offset = static_cast<uint>(leaf_prim_ids.size());
                leaf_prim_ids.insert(leaf_prim_ids.end(), overlap_prim_ids,
                                     overlap_prim_ids + n_prims);
        }
    }

    template <class Primitive>
    KdTree<Primitive>::Node::Node(const uint split_axis, const float split_loc,
                                  const uint node_above): is_leaf{0}, split_pos{split_loc} {
        interior.axis = split_axis;
        interior.above_child = node_above;
    }

    template <class Primitive>
    bool KdTree<Primitive>::Node::isLeaf() const {
        return is_leaf == 1;
    }

    template <class Primitive>
    bool KdTree<Primitive>::Node::isInterior() const {
        return is_leaf == 0;
    }

    template <class Primitive>
    uint KdTree<Primitive>::Node::splitAxis() const {
        assert(isInterior());
        return interior.axis;
    }

    template <class Primitive>
    uint KdTree<Primitive>::Node::nPrimitives() const {
        assert(isLeaf());
        return leaf.n_prims;
    }

    template <class Primitive>
    uint KdTree<Primitive>::Node::topChild() const {
        assert(isInterior());
        return interior.above_child;
    }

    template <class Primitive>
    KdTree<Primitive>::BoundEdge::BoundEdge(const float axis_position, const uint prim_index,
                                            const Type edge_type): axis_pos{axis_position},
                                            prim_id{prim_index}, type{edge_type} {}

    template <class Primitive>
    bool KdTree<Primitive>::BoundEdge::operator<(const BoundEdge& rhs) const {
        if (axis_pos != rhs.axis_pos) {
            return axis_pos < rhs.axis_pos;
        } else {
            // END before START before BOTH
            return type < rhs.type;
        }
    }

    template <class Primitive>
    KdTree<Primitive>::TraceNode::TraceNode(const float t_entry, const float t_exit,
                                            const uint node_id): t_min{t_entry}, t_max{t_exit},
                                            id{node_id} {}

    template <class Primitive>
    KdTree<Primitive>::KdTree(const std::vector<Primitive>& primitives, const int inters_cost,
                              const int trav_cost, const int max_depth, const uint min_num_prims,
                              const float empty_bonus): m_max_depth{max_depth}, m_actual_depth{0},
                              m_min_num_prims{glm::max(min_num_prims, 1u)},
                              m_avg_prims_per_leaf{0.0f}, m_inters_cost{inters_cost},
                              m_trav_cost{trav_cost}, m_empty_bonus{empty_bonus},
                              m_next_node_id{0}, m_n_alloc_nodes{0}, m_leaf_count{0},
                              m_prims{primitives.data()} {
        const uint n{static_cast<uint>(primitives.size())};
        if (m_max_depth < 0) {
            // Maximal depth = 8 + 1.3 * log2(n)
            m_max_depth = static_cast<int>(8 + 1.3f * log2f(static_cast<float>(n)));
        }
        printInfo("Constructing k-d tree for %u primitives.", n);
        printInfo("Maximal tree depth: %i.", m_max_depth);
        const auto t_begin = HighResTimer::now();
        // Reserve memory
        m_prim_boxes = new BBox[n];
        uint* prims_bottom{new uint[n]};
        uint* prims_top{new uint[n * (m_max_depth + 1)]};
        for (auto axis = 0; axis < 3; ++axis) { m_edges[axis] = new BoundEdge[2 * n]; }
        // Each primitive overlaps the root node
        for (uint i = 0; i < n; ++i) {
            prims_bottom[i] = i;
            m_prim_boxes[i] = m_prims[i].computeBBox();
            m_tree_box.extend(m_prim_boxes[i]);
        }
        // Recursively build the tree
        buildTree(0, m_tree_box, prims_bottom, n, 0, 0, prims_bottom, prims_top);
        // Cleaning up
        m_nodes.resize(m_next_node_id);
        m_nodes.shrink_to_fit();
        for (auto axis = 0; axis < 3; ++axis) { delete[] m_edges[axis]; }
        delete[] prims_top;
        delete[] prims_bottom;
        delete[] m_prim_boxes;
        // Compute and print statistics
        m_avg_prims_per_leaf /= m_leaf_count;
        const auto t_diff = HighResTimer::now() - t_begin;
        const auto t_us   = std::chrono::duration_cast<std::chrono::microseconds>(t_diff).count();
        printInfo("k-d tree construction complete after %.2f ms.", t_us * 0.001f);
        printInfo("Actual tree depth: %i.", m_actual_depth);
        printInfo("Average number of primitives per leaf: %.2f.", m_avg_prims_per_leaf);
        printInfo("k-d tree consists of %u nodes.", m_next_node_id);
    }

    template <class Primitive>
    const BBox& KdTree<Primitive>::bbox() const {
        return m_tree_box;
    }

    template <class Primitive>
    void KdTree<Primitive>::buildTree(const uint node_id, const BBox& node_box,
                                      const uint* overlap_prim_ids, const uint n_prims,
                                      const int depth, int bad_refines,
                                      uint* prims_bottom, uint* prims_top) {
        // Get next free node
        if (m_next_node_id == m_n_alloc_nodes) {
            // Use growth rate factor of 1.5 for efficiency
            m_n_alloc_nodes = glm::max((m_n_alloc_nodes * 3) / 2, 64u);
            m_nodes.resize(m_n_alloc_nodes);
        }
        ++m_next_node_id;
        m_actual_depth = glm::max(m_actual_depth, depth + 1);
        if (n_prims <= m_min_num_prims || depth == m_max_depth) {
            // Initialize leaf node
            m_nodes[node_id] = Node{n_prims, overlap_prim_ids, m_leaf_prim_ids};
            m_avg_prims_per_leaf += n_prims;
            ++m_leaf_count;
            return;
        } else {
            // Initialize interior node
            float best_cost    = FLT_MAX;       // Best splitting cost
            uint  best_axis    = UINT32_MAX;	// Best axis to perform split along
            uint  best_edge_id = UINT32_MAX;	// Best bounding edge index for splitting
            const float inv_total_sa{1.0f / node_box.computeArea()};
            const vec3 diag{node_box.dimensions()};
            // Sort all three axes in parallel; slower, but produces the highest quality tree
            #pragma omp parallel for num_threads(3)
            for (int ax = 0; ax < 3; ++ax) {
                for (uint i = 0; i < n_prims; ++i) {
                    const uint  prim_id{overlap_prim_ids[i]};
                    const BBox& primBox{m_prim_boxes[prim_id]};
                    // Handle special case of primitives parallel to splitting plane
                    const bool is_flat{primBox.minPt()[ax] == primBox.maxPt()[ax]};
                    m_edges[ax][2 * i    ] = BoundEdge{primBox.minPt()[ax], prim_id,
                                             is_flat ? BoundEdge::BOTH : BoundEdge::START};
                    m_edges[ax][2 * i + 1] = BoundEdge{primBox.maxPt()[ax], prim_id,
                                             is_flat ? BoundEdge::NONE : BoundEdge::END};
                }
                // Sort edges for this axis
                std::sort(&m_edges[ax][0], &m_edges[ax][2 * n_prims]);
            }
            for (int axis = 0; axis < 3; ++axis) {
                // At the beginning all primitives are above split line
                uint n_below{0};
                uint n_above{n_prims};
                // Compute split costs for axis; iterate over all edges from bottom to top
                for (uint i = 0; i < 2 * n_prims; ++i) {
                    switch (m_edges[axis][i].type) {
                        case BoundEdge::BOTH:  ++n_below;
                        case BoundEdge::END:   --n_above;
                        case BoundEdge::START: break;
                        case BoundEdge::NONE:  continue;
                    }
                    const float edge_pos{m_edges[axis][i].axis_pos};
                    const bool is_inside_node_box{edge_pos > node_box.minPt()[axis] &&
                                                  edge_pos < node_box.maxPt()[axis]};
                    if (is_inside_node_box) {
                        // Compute cost split for edge i
                        const int axis2{(axis + 1) % 3};
                        const int axis3{(axis + 2) % 3};
                        const float below_sa{2 * (diag[axis2] * diag[axis3] +
                            (edge_pos - node_box.minPt()[axis]) * (diag[axis2] + diag[axis3]))};
                        const float above_sa{2 * (diag[axis2] * diag[axis3] +
                            (node_box.maxPt()[axis] - edge_pos) * (diag[axis2] + diag[axis3]))};
                        const float bns{(0 == n_above || 0 == n_below) ? m_empty_bonus : 0.0f};
                        const float cost{(1.0f - bns) * (n_below * below_sa + n_above * above_sa)};
                        // Determine whether this is the best split so far
                        if (cost < best_cost) {
                            best_axis    = axis;
                            best_cost    = cost;
                            best_edge_id = i;
                        }
                    }
                    if (BoundEdge::START == m_edges[axis][i].type) { ++n_below; }
                }
            }
            // Adjust the best cost to account for individual node
            // traversal and primitive intersection costs
            best_cost = m_trav_cost + (m_inters_cost * inv_total_sa) * best_cost;
            const uint old_cost{m_inters_cost * n_prims};
            if (best_cost > old_cost) { ++bad_refines; }
            if (best_cost > 4.0f * old_cost && n_prims < 16 || 3 == bad_refines) {
                // Initialize leaf node
                m_nodes[node_id] = Node{n_prims, overlap_prim_ids, m_leaf_prim_ids};
                m_avg_prims_per_leaf += n_prims;
                ++m_leaf_count;
                return;
            }
            // Assign primitives
            uint n_bottom{0}, n_top{0};
            // Bottom
            for (uint k = 0; k < best_edge_id; ++k) {
                if (BoundEdge::START == m_edges[best_axis][k].type ||
                    BoundEdge::BOTH  == m_edges[best_axis][k].type) {
                    prims_bottom[n_bottom++] = m_edges[best_axis][k].prim_id;
                }
            }
            // Split
            if (BoundEdge::BOTH == m_edges[best_axis][best_edge_id].type) {
                prims_bottom[n_bottom++] = m_edges[best_axis][best_edge_id].prim_id;
            }
            // Top
            for (uint k = best_edge_id + 1; k < 2 * n_prims; ++k) {
                if (BoundEdge::END  == m_edges[best_axis][k].type ||
                    BoundEdge::BOTH == m_edges[best_axis][k].type) {
                    prims_top[n_top++] = m_edges[best_axis][k].prim_id;
                }
            }
            // Recursively initialise children
            const float split_pos{m_edges[best_axis][best_edge_id].axis_pos};
            vec3 top_min_pt{node_box.minPt()}, bottom_max_pt{node_box.maxPt()};
            top_min_pt[best_axis] = bottom_max_pt[best_axis] = split_pos;
            const BBox bottom_box{node_box.minPt(), bottom_max_pt};
            // Bottom child
            buildTree(node_id + 1, bottom_box, prims_bottom, n_bottom, depth + 1, bad_refines,
                      prims_bottom, prims_top + n_prims);
            const uint top_child_id{m_next_node_id};
            // Insert an interior node
            m_nodes[node_id] = Node{best_axis, split_pos, top_child_id};
            // Top child
            const BBox top_box{top_min_pt, node_box.maxPt()};
            buildTree(top_child_id, top_box, prims_top, n_top, depth + 1, bad_refines,
                      prims_bottom, prims_top + n_prims);
        }
    }

    template <class Primitive>
    bool KdTree<Primitive>::intersect(Ray& ray, const bool is_vis_ray) const {
        // Intersect with tree's BBox first
        const auto is = m_tree_box.intersect(ray);
        if (!is) {
            return false; // No intersection
        }
        ray.setValidRange(glm::max(ray.t_min, is.entr - getRescaledEPS(is.entr)),
                          glm::min(ray.t_max, is.exit + getRescaledEPS(is.exit)));
        // Using stack-based traversal
        TraceNode trace_stack[32];
        int stack_pos{-1};
        const Node* node{&m_nodes[0]};
        while (nullptr != node) {
            if (ray.inters.distance < ray.t_min) {
                // The old intersection is closer to origin of ray
                break;
            }
            if (node->isInterior()) {
                const uint axis{node->splitAxis()};
                // Find both children, top and bottom
                const Node* children[2] = {&m_nodes[node->topChild()], node + 1};
                const bool bottom_first{(ray.o[axis] <  node->split_pos) ||
                                        (ray.o[axis] == node->split_pos && ray.d[axis] <= 0.0f)};
                if (bottom_first) {
                    std::swap(children[0], children[1]);
                }
                // Determine distance to split plane
                const float t_split{(node->split_pos - ray.o[axis]) * ray.inv_d[axis]};
                if (t_split > ray.t_max || t_split <= 0.0f) {
                    // We have to visit the 1st child ONLY
                    node = children[0];
                } else if (t_split < ray.t_min) {
                    // We have to visit the 2nd child ONLY
                    node = children[1];
                } else {
                    // We have to visit BOTH children
                    // We add the 2nd child to the stack
                    const uint second_child_id{static_cast<uint>(children[1] - &m_nodes[0])};
                    const float new_t_min{t_split - getRescaledEPS(t_split)};
                    assert(stack_pos < 31);
                    trace_stack[++stack_pos] = TraceNode{new_t_min, ray.t_max, second_child_id};
                    // And then start with the closest node - the 1st child
                    node      = children[0];
                    ray.t_max = t_split + getRescaledEPS(t_split);
                }
            } else {
                // Leaf node
                const uint n_prims{node->nPrimitives()};
                if (1u == n_prims) {
                    // Intersect this primitive
                    const Primitive& prim{m_prims[node->single_prim_id]};
                    if (prim.intersect(ray)) {
                        // Intersection found
                        return true;
                    }
                } else {
                    bool hit{false};
                    for (uint i = 0; i < n_prims; ++i) {
                        const Primitive& prim{m_prims[m_leaf_prim_ids[node->prims_offset + i]]};
                        if (prim.intersect(ray)) {
                            // Intersection found
                            if (is_vis_ray) { return true; }
                            hit = true;
                        }
                    }
                    if (hit) { return true; }
                }
                if (stack_pos >= 0) {
                    // Get the next node
                    node      = &m_nodes[trace_stack[stack_pos].id];
                    ray.t_min = trace_stack[stack_pos].t_min;
                    ray.t_max = trace_stack[stack_pos].t_max;
                    --stack_pos;
                } else {
                    // Nothing left to traverse
                    break;
                }
            }
        }
        return false;
    }
}
