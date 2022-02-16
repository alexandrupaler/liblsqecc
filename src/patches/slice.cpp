#include <lsqecc/patches/slice.hpp>
#include <iterator>
#include <algorithm>

namespace lsqecc{

Slice Slice::make_copy_with_cleared_activity() const {
    Slice new_slice;

    // Remove patches that were measured in the previous timestep
    std::ranges::copy_if(patches,
            std::back_inserter(new_slice.patches),
            [](const Patch& p){return p.activity!=PatchActivity::Measurement;});
    for (auto& e : new_slice.patches) {
        // Clear Unitary Operator activity
        if(e.activity == PatchActivity::Unitary)
            e.activity = PatchActivity::None;
    }

    return new_slice;
}

Patch& Slice::get_patch_by_id(PatchId id) {
    for(auto& p: patches)
    {
        if(p.id == id)
        {
            return p;
        }
    }
}

Cell Slice::get_furthest_cell() const
{
    Cell ret{0,0};
    for(const Patch& p: patches)
    {
        for (const Cell& c: p.get_cells())
        {
            ret.col = std::max(ret.col, c.col);
            ret.row = std::max(ret.row, c.row);
        }
    }
    return ret;
}

}
