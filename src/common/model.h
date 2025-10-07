#pragma once

class Model
{
    struct IDrawable
    {
        virtual ~IDrawable() = default;
        virtual std::unique_ptr<IDrawable> copy_() const = 0;
        virtual void touch_(void*) const = 0;
    };

    template <typename T>
    struct DrawableObject final : IDrawable 
    {
        T data_;

        DrawableObject(T x)
            : data_(std::move(x))
        {}

        std::unique_ptr<IDrawable> copy_() const override {
            return std::make_unique<DrawableObject>(*this);
        }

        void touch_(void* user) const override 
        {
            ::touch(data_, user);
        }
    };

    std::unique_ptr<IDrawable> self_;

public:

    template <typename T>
    Model(T x)
        : self_(std::make_unique<DrawableObject<T>>(std::move(x))) 
    {}

    // copy ctor, move ctor and assignment
public:

    Model(const Model &x)
        : self_(x.self_->copy_()) 
    {}

    Model(Model &&x) noexcept = default;

    Model &operator=(Model x) noexcept
    {
        self_ = std::move(x.self_);
        return *this;
    }

public:

    void touch(void *user) const
    {
        self_->touch_(user);
    }
};

