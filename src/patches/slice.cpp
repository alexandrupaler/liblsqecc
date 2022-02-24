#include <lsqecc/patches/slice.hpp>
#include <iterator>
#include <algorithm>
#include <iostream>

namespace lsqecc{


std::optional<Cell> Slice::find_place_for_magic_state(size_t distillation_region_idx) const
{
    for(const auto& cell: layout.distilled_state_locations(distillation_region_idx))
        if(is_cell_free(cell))
            return cell;

    return std::nullopt;
}



Patch& Slice::get_patch_by_id_mut(PatchId id) {
    for(auto& p: qubit_patches)
    {
        if(p.id == id)
        {
            return p;
        }
    }
    throw std::logic_error(std::string{"No patch for id: "+std::to_string(id)});
}

const Patch& Slice::get_patch_by_id(PatchId id) const {
    for(auto& p: qubit_patches)
    {
        if(p.id == id)
        {
            return p;
        }
    }
    throw std::logic_error(std::string{"No patch for id: "+std::to_string(id)});
}


std::optional<std::reference_wrapper<const Patch>> Slice::get_qubit_patch_on_cell(const Cell& cell) const
{
    for(const Patch& p: qubit_patches)
        for (const Cell& c: p.get_cells())
            if(c==cell)
                return p;

    return std::nullopt;
}


std::optional<std::reference_wrapper<const Patch>> Slice::get_magic_state_on_cell(const Cell& cell) const
{
    for(const Patch& p: unbound_magic_states)
        for (const Cell& c: p.get_cells())
            if(c==cell)
                return p;

    return std::nullopt;
}

std::optional<std::reference_wrapper<const Patch>> Slice::get_any_patch_on_cell(const Cell& cell) const
{
    auto p = get_qubit_patch_on_cell(cell);
    if(p) return p;
    return get_magic_state_on_cell(cell);
}

bool Slice::is_cell_free(const Cell& cell) const
{
    for(const auto& distillation_region: layout.distillation_regions())
        for(const auto& distillation_cell: distillation_region.sub_cells)
            if(distillation_cell.cell==cell)
                return false;

    return !Slice::get_any_patch_on_cell(cell);
}

std::vector<Cell> Slice::get_neigbours_within_slice(const Cell& cell) const
{
    return cell.get_neigbours_within_bounding_box_inclusive({0,0}, layout.furthest_cell());
}

}
