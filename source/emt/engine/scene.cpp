#include "scene.h"
#include <emt/graphics/dx/dx_context_core.h>

namespace emt
{
scene::scene(dx_context_core* context) :
    m_context(context), m_device(context->graphic_device())
{
}

}        // namespace emt
