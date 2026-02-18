#include "error-mask.hpp"

error_mask::error_mask()
    : eng::sibus::node("error-mask")
{
    error_ = node::add_output_port("error");

    for (std::size_t i = 0; i < ebits_.size(); ++i)
    {
        node::add_input_port_unsafe(std::format("b{}", i), [this, i](eng::abc::pack args)
        {
            bool value = args ? eng::abc::get<bool>(args) : true;
            ebits_.set(i, value);
            update_output(ebits_.any());
        });
    }
}

void error_mask::update_output(bool value)
{
    if (value == result_)
        return;
    result_ = value;
    node::set_port_value(error_, { result_ });
}
