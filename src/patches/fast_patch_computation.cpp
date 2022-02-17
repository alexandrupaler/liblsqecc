#include <lsqecc/patches/patches.hpp>
#include <lsqecc/patches/fast_patch_computation.hpp>


#include <stdexcept>
#include <iterator>
#include <ranges>
#include <iostream>

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/property_map/property_map.hpp>


namespace lsqecc {


class SimpleLayout : public Layout {
public:
    explicit SimpleLayout(size_t num_qubits) : num_qubits_(num_qubits) {}

    const std::vector<Patch> core_patches() const override
    {
        std::vector<Patch> core;
        core.reserve(num_qubits_);
        for(size_t i = 0; i<num_qubits_; i++)
        {
            core.push_back(basic_square_patch({
                    .row=0,
                    .col=2*static_cast<Cell::CoordinateType>(i)
            }));
        }
        return core;
    }

    std::vector<Cell> magic_state_queue_locations() const override {
        std::vector<Cell> magic_state_queue;
        magic_state_queue.reserve(num_qubits_);
        for(size_t i = 0; i<num_qubits_; i++)
        {
            magic_state_queue.push_back({
                    .row=0,
                    .col=2*static_cast<Cell::CoordinateType>(i)
            });
        }
        return magic_state_queue;
    }
    std::vector<Cell> distillery_locations()  const override
    {
        return {};
    };


private:
    static Patch basic_square_patch(Cell placement){
        return Patch{
                .cells=SingleCellOccupiedByPatch{
                        .top={BoundaryType::Rough,false},
                        .bottom={BoundaryType::Rough,false},
                        .left={BoundaryType::Smooth,false},
                        .right={BoundaryType::Smooth,false},
                        .cell=placement
                },
                .type=PatchType::Qubit,
                .id=std::nullopt,
        };
    }

private:
    size_t num_qubits_;

};



Slice first_slice_from_layout(const Layout& layout)
{
    Slice slice{.distance_dependant_timesteps=1, .patches={}};

    for (const Patch& p : layout.core_patches())
        slice.patches.push_back(p);

    return slice;
}



RoutingRegion graph_search_route_ancilla(
        const Slice& slice,
        PatchId source,
        PauliOperator source_op,
        PatchId target,
        PauliOperator target_op
        )
{

    using namespace boost;


    using Vertex = adjacency_list_traits <vecS, vecS, directedS >::vertex_descriptor;
    using Graph = adjacency_list<vecS, vecS, directedS, property< vertex_predecessor_t, Vertex >, property< edge_weight_t, int >>;
    using Edge = std::pair<Vertex, Vertex>;

    std::vector<Vertex> vertices;
    std::vector<Edge> edges;

    Cell furthest_cell = slice.get_furthest_cell();

    auto make_vertex = [&furthest_cell](const Cell& cell) -> Vertex{
        return cell.row*(furthest_cell.col+1) + cell.col;
    };

    auto cell_from_vertex = [&furthest_cell](Vertex vertex) -> Cell {
        auto v = static_cast<Cell::CoordinateType>(vertex);
        auto col = v % (furthest_cell.col+1);
        return Cell{(v-col)/(furthest_cell.col+1), col};
    };


    // Add free node
    for (Cell::CoordinateType row_idx = 0; row_idx<=furthest_cell.row; ++row_idx)
    {
        for (Cell::CoordinateType col_idx = 0; col_idx<=furthest_cell.col; ++col_idx)
        {
            Cell current{row_idx, col_idx};
            vertices.push_back(make_vertex(current));
            std::optional<std::reference_wrapper<const Patch>> patch_of_node{slice.get_patch_on_cell(current)};

            bool node_is_free = !patch_of_node;
            if (node_is_free)
            {
                // Always fill edges going in for empty
                for (const Cell& neighbour: current.get_neigbours())
                {
                    if (neighbour.row>=0 &&
                            neighbour.row<=furthest_cell.row &&
                            neighbour.col>=0 &&
                            neighbour.col<=furthest_cell.col &&
                            !slice.get_patch_on_cell(neighbour)
                            )
                    {
                        edges.emplace_back(make_vertex(neighbour), make_vertex(current));
                    }
                }
            }
        }
    }

    // Add source
    decltype(auto) source_patch = std::get_if<SingleCellOccupiedByPatch>(&slice.get_patch_by_id(source).cells);
    if (source_patch == nullptr) throw std::logic_error("Cannot route multi cell patches");
    for(const Cell& neighbour: source_patch->cell.get_neigbours())
    {
        auto boundary = source_patch->get_boundary_with(neighbour);
        if(boundary && boundary->boundary_type == boundary_for_operator(source_op)){
            edges.emplace_back(make_vertex(source_patch->cell), make_vertex(neighbour));
        }
    }

    // Add target
    decltype(auto) target_patch = std::get_if<SingleCellOccupiedByPatch>(&slice.get_patch_by_id(target).cells);
    if (target_patch == nullptr) throw std::logic_error("Cannot route multi cell patches");
    for(const Cell& neighbour: target_patch->cell.get_neigbours())
    {
        auto boundary = target_patch->get_boundary_with(neighbour);
        if(boundary && boundary->boundary_type == boundary_for_operator(target_op)){
            edges.emplace_back(make_vertex(neighbour), make_vertex(target_patch->cell));
        }
    }

    Graph g{edges.begin(), edges.end(), vertices.size()};


    property_map< Graph, vertex_predecessor_t >::type p
            = get(vertex_predecessor, g);

    Vertex s = vertex(make_vertex(slice.get_patch_by_id(source).get_a_cell()), g);
    dijkstra_shortest_paths(g, s, predecessor_map(p));

#if false
    std::ofstream dotfile ("graph");
    write_graphviz(dotfile, g);
    std::cout<<"S:"<<make_vertex(slice.get_patch_by_id(source).get_cells()[0])<<" "
             <<"T:"<<make_vertex(slice.get_patch_by_id(target).get_cells()[0])<<std::endl;
    for(int i = 0; i<vertices.size(); i++){
        std::cout << i << "->" << p[i] <<std::endl;
        std::cout << cell_from_vertex(i).row << ", " << cell_from_vertex(i).col
                  << "->"
                  << cell_from_vertex(p[i]).row << ", " << cell_from_vertex(p[i]).col <<std::endl;
    }
#endif

    RoutingRegion ret;

    Vertex prec = make_vertex(slice.get_patch_by_id(target).get_a_cell());
    Vertex curr = p[prec];
    Vertex next = p[curr];
    while(curr!=next)
    {
        Cell prec_cell = cell_from_vertex(prec);
        Cell curr_cell = cell_from_vertex(curr);
        Cell next_cell = cell_from_vertex(next);

        ret.cells.push_back(SingleCellOccupiedByPatch{
            .top=   {BoundaryType::None, false},
            .bottom={BoundaryType::None, false},
            .left=  {BoundaryType::None, false},
            .right= {BoundaryType::None, false},
            .cell=curr_cell
        });

        for(const Cell& neighbour : curr_cell.get_neigbours())
        {
            if(prec_cell==neighbour || next_cell==neighbour)
            {
                auto boundary = ret.cells.back().get_mut_boundary_with(neighbour);
                if(boundary) boundary->get() = {.boundary_type=BoundaryType::Connected, .is_active=true};
            }
        }

        prec = curr;
        curr = next;
        next = p[next];
    }

    return ret;
}




PatchComputation PatchComputation::make(const LogicalLatticeComputation& logical_computation) {
    PatchComputation patch_computation;
    patch_computation.layout = std::make_unique<SimpleLayout>(logical_computation.core_qubits.size());
    patch_computation.slices.push_back(first_slice_from_layout(*patch_computation.layout));
    auto& patches = patch_computation.slices[0].patches;
    auto& ids = logical_computation.core_qubits;
    if(patches.size() < ids.size()){
        throw std::logic_error("Not enough patches for all ids");
    }

    auto patch_itr = patches.begin();
    for (auto id : ids)
    {
        (patch_itr++)->id = id;
    }

    for(const LogicalLatticeOperation& instruction : logical_computation.instructions)
    {
        Slice& slice = patch_computation.new_slice();

        if (const auto* s = std::get_if<SinglePatchMeasurement>(&instruction.operation))
        {
            slice.get_patch_by_id_mut(s->target).activity = PatchActivity::Measurement;
        }
        else if (const auto* p = std::get_if<LogicalPauli>(&instruction.operation))
        {
            slice.get_patch_by_id_mut(p->target).activity = PatchActivity::Unitary;
        }
        else if (const auto* m = std::get_if<MultiPatchMeasurement>(&instruction.operation))
        {
            if(m->observable.size()!=2)
                throw std::logic_error("Multi patch measurement only supports 2 patches currently");
            auto pairs = m->observable.begin();
            const auto& [source_id, source_op] = *pairs++;
            const auto& [target_id, target_op] = *pairs;
            slice.routing_regions.push_back(
                    graph_search_route_ancilla(slice, source_id, source_op, target_id, target_op)
            );
        }
        else
        {
            const auto& mr = std::get<MagicStateRequest>(instruction.operation);
        }
    }


    return patch_computation;
}


size_t PatchComputation::num_slices() const
{
    return slices.size();
}


const Slice& PatchComputation::slice(size_t idx) const
{
    return slices.at(idx);
}

const Slice& PatchComputation::last_slice() const {
    return slice(num_slices()-1);
}

Slice& PatchComputation::new_slice() {
    slices.emplace_back(last_slice().make_copy_with_cleared_activity());
    return slices.back();
}


}

