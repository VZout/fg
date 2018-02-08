#include "catch.hpp"

#include <gl/buffer.hpp>
#include <gl/texture.hpp>

#include <fg/framegraph.hpp>

namespace glr
{
struct buffer_description
{
  std::size_t size;
};
struct texture_description
{
  std::size_t                levels;
  GLenum                     format;
  std::array<std::size_t, 3> size  ;
};

using buffer_resource     = fg::resource<buffer_description , gl::buffer    >;
using texture_1d_resource = fg::resource<texture_description, gl::texture_1d>;
using texture_2d_resource = fg::resource<texture_description, gl::texture_2d>;
using texture_3d_resource = fg::resource<texture_description, gl::texture_3d>;
}

namespace fg
{
template<>
std::unique_ptr<gl::buffer>     realize(const glr::buffer_description&  description)
{
  auto   actual = std::make_unique<gl::buffer>(); 
  actual->set_data_immutable(static_cast<GLsizeiptr>(description.size));
  return actual;
}
template<>
std::unique_ptr<gl::texture_2d> realize(const glr::texture_description& description)
{
  auto   actual = std::make_unique<gl::texture_2d>();
  actual->set_storage(static_cast<GLsizei>(description.levels), description.format, static_cast<GLsizei>(description.size[0]), static_cast<GLsizei>(description.size[1]));
  return actual;
}
}

TEST_CASE("Framegraph test.", "[framegraph]")
{
  fg::framegraph framegraph;

  auto retained_resource = framegraph.add_retained_resource<glr::texture_description, gl::texture_2d>("Retained Resource 1", glr::texture_description(), nullptr);

  // First render task declaration.
  struct render_task_1_data
  {
    glr::texture_2d_resource* output1;
    glr::texture_2d_resource* output2;
    glr::texture_2d_resource* output3;
    glr::texture_2d_resource* output4;
  };
  auto render_task_1 = framegraph.add_render_task<render_task_1_data>(
    "Render Task 1",
    [&] (render_task_1_data& data, fg::render_task_builder& builder)
    {
      data.output1 = builder.create<glr::texture_2d_resource>("Resource 1", glr::texture_description());
      data.output2 = builder.create<glr::texture_2d_resource>("Resource 2", glr::texture_description());
      data.output3 = builder.create<glr::texture_2d_resource>("Resource 3", glr::texture_description());
      data.output4 = builder.write <glr::texture_2d_resource>(retained_resource);
    },
    [=] (const render_task_1_data& data)
    {
      // Perform actual rendering. You may load resources from CPU by capturing them.
      // data.output->actual()->bind();
    });

  auto& data_1 = render_task_1->data();
  REQUIRE(data_1.output1->id() == 1);
  REQUIRE(data_1.output2->id() == 2);
  REQUIRE(data_1.output3->id() == 3);
  
  // Second render pass declaration.
  struct render_task_2_data
  {
    glr::texture_2d_resource* input ;
    glr::texture_2d_resource* output;
  };
  auto render_task_2 = framegraph.add_render_task<render_task_2_data>(
    "Render Task 2",
    [&] (render_task_2_data& data, fg::render_task_builder& builder)
    {
      data.input  = builder.read                            (data_1.output1);
      data.input  = builder.read                            (data_1.output2);
      data.input  = builder.write                           (data_1.output3);
      data.output = builder.create<glr::texture_2d_resource>("Resource 4", glr::texture_description());
    },
    [=] (const render_task_2_data& data)
    {
      // Perform actual rendering. You may load resources from CPU by capturing them.
      // data.input->actual()->bind();
    });

  auto& data_2 = render_task_2->data();
  REQUIRE(data_2.output->id() == 4);
  
  framegraph.compile        ();
  for(auto i = 0; i < 100; i++)
    framegraph.execute      ();
  framegraph.export_graphviz("framegraph.gv");
  framegraph.clear          ();
}