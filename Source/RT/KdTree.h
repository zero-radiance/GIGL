#pragma once

#include <vector>
#include "..\Common\BBox.h"

namespace rt {
    /* k-d tree class (spatial subdivision acceleration structure) */
    template <class Primitive>
    class KdTree {
    public:
        KdTree() = delete;
        RULE_OF_ZERO(KdTree);
        // Builds k-d tree using all scene primitives
        explicit KdTree(const std::vector<Primitive>& primitives, const int inters_cost,
                        const int trav_cost, const int max_depth, const uint min_num_prims,
                        const float empty_bonus = 0.25f);
        // Returns bounding box encompassing all primitives
        const BBox& bbox() const;
        // Front-to-back traversal algorithm
        bool intersect(Ray& ray, const bool is_vis_ray = false) const;
    private:
        // 8 byte node
        class Node {
        public:
            // Leaf node (empty)
            Node();
            RULE_OF_ZERO(Node);
            // Leaf node, at least one primitive
            explicit Node(const uint n_prims, const uint* overlap_prim_ids,
                          std::vector<uint>& leaf_prim_ids);
            // Interior node
            explicit Node(const uint split_axis, const float split_loc, const uint node_above);
            // Checks for leaf flag
            bool isLeaf() const;
            // Checks for interior node flag
            bool isInterior() const;
            // Returns split axis of inner node
            uint splitAxis() const;
            // Returns number of primitives within leaf
            uint nPrimitives() const;
            // Returns index of neighbour inner node
            uint topChild() const;
            // 4 bytes
            union {
                float split_pos;		    // Split position along axis (interior only)
                uint  single_prim_id;       // Store single primitive index directly (leaves only)
                uint  prims_offset;         // Offset into "m_leaf_prim_ids" (leaves only)
            };
        private:
            // 4 bytes
            union {
                uint is_leaf         : 1;	// Leaf flag (both interior and leaf nodes)
                // Interior only
                struct {
                    uint is_leaf     : 1;   // == 0
                    uint axis        : 2;	// Split axis
                    uint above_child : 29;	// Index of child node above the split plane
                } interior;
                // Leaves only
                struct {
                    uint is_leaf     : 1;   // == 1
                    uint n_prims     : 31;  // Number of primitives overlapping node
                } leaf;
            };
        };
        /* Bounding edge of node */
        struct BoundEdge {
            enum Type { END = 0, START = 1, BOTH = 2, NONE = 3 };
            BoundEdge() = default;
            RULE_OF_ZERO(BoundEdge);
            explicit BoundEdge(const float axis_position, const uint prim_index,
                               const Type edge_type);
            bool operator<(const BoundEdge& rhs) const;
            float  axis_pos;	            // Position along axis
            uint   prim_id;	                // Triangle index within "m_prims" vector
            Type   type;	                // Bounding edge type
        };
        /* Element of k-d tree traversal stack */
        struct TraceNode {
            TraceNode() = default;
            RULE_OF_ZERO(TraceNode);
            explicit TraceNode(const float t_entry, const float t_exit, const uint node_id);
            float t_min, t_max;			    // Distances to entry and exit points
            uint  id;                       // Node index within "m_nodes"
        };
        // Private data members
        int   m_max_depth;					// Max. possible tree depth
        int   m_actual_depth;				// Max. depth of the actual tree
        uint  m_min_num_prims;				// Min. number of prims for instant leaf creation
        float m_avg_prims_per_leaf;			// Average number of primitives per leaf
        int   m_inters_cost;				// Intersection cost
        int   m_trav_cost;					// Traversal cost
        float m_empty_bonus;				// Cost reduction bonus for empty nodes
        BBox  m_tree_box;					// Bounding box containing the entire tree
        uint  m_next_node_id;				// Current node array index
        uint  m_n_alloc_nodes;			    // Size of allocated node array
        uint  m_leaf_count;				    // Number of leaf nodes
        std::vector<Node> m_nodes;		    // Vector of nodes, from bottom to top
        std::vector<uint> m_leaf_prim_ids;  // Indices of primitives contained by leaves
        const Primitive*  m_prims;          // All primitives contained by the tree
        // Temporary storage
        BBox*      m_prim_boxes;            // Bounding boxes of all primitives
        BoundEdge* m_edges[3];              // Bounding edges of all primitives
        // Recursive builder function
        void buildTree(const uint node_id, const BBox& node_box, const uint* overlap_prim_ids,
                       const uint n_prims, const int depth, int bad_refines,
                       uint* prims_bottom, uint* prims_top);
    };

    class Triangle;
    using KdTri = KdTree<Triangle>;
}
