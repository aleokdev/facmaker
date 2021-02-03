#include <imgui.h>
#include <vector>

#include "factory.hpp"

namespace imnodes {

struct EditorContext;

}
namespace fmk {

class FactoryEditor {
public:
    FactoryEditor();
    void draw();

private:
    void update_processing_graph();
    void update_text_editor();

    struct Cache {
        std::vector<ImVec2> machine_positions;

        mutable bool is_dirty = true;
    } cache;

    Factory factory;
    imnodes::EditorContext* imnodes_ctx;
};

} // namespace fmk