#include <type_traits>
#include <utility>

template <typename> class Function;

template <typename Ret, typename... Args> class Function<Ret(Args...)>
{
  public:
    // Конструктор по умолчанию
    Function() = default;

    // Конструктор из вызываемого объекта
    template <typename F, typename = std::enable_if_t<
                              std::is_invocable_r_v<Ret, F, Args...> &&
                              !std::is_same_v<std::decay_t<F>, Function>>>
    Function(F &&f)
        : callable_(new Callable<std::decay_t<F>>(std::forward<F>(f)))
    {
    }

    // Конструктор копирования
    Function(const Function &other)
        : callable_(other.callable_ ? other.callable_->clone() : nullptr)
    {
    }

    // Конструктор перемещения
    Function(Function &&other) noexcept : callable_(other.callable_)
    {
        other.callable_ = nullptr;
    }

    // Оператор присваивания
    Function &operator=(Function other)
    {
        swap(other);
        return *this;
    }

    // Деструктор
    ~Function() { delete callable_; }

    // Проверка на наличие целевого объекта
    explicit operator bool() const noexcept { return callable_ != nullptr; }

    // Вызов функции
    Ret operator()(Args... args) const
    {
        return callable_->invoke(std::forward<Args>(args)...);
    }

    void swap(Function &other) noexcept
    {
        std::swap(callable_, other.callable_);
    }

  private:
    struct CallableBase {
        virtual ~CallableBase() = default;
        virtual Ret invoke(Args &&...args) = 0;
        virtual CallableBase *clone() const = 0;
    };

    template <typename F> struct Callable : CallableBase {
        explicit Callable(F &&f) : f_(std::forward<F>(f)) {}

        Ret invoke(Args &&...args) override
        {
            return f_(std::forward<Args>(args)...);
        }

        CallableBase *clone() const override { return new Callable<F>(f_); }

        F f_;
    };

    CallableBase *callable_ = nullptr;
};
