#pragma once

#include <memory>

class Object : public std::enable_shared_from_this<Object> {
  public:
    using NodeType = std::shared_ptr<Object>;

    virtual ~Object() = default;

    virtual bool Callable() const { return false; }

    virtual std::shared_ptr<Object> Call(NodeType args = nullptr) = 0;

  private:
    friend std::ostream &operator<<(std::ostream &out,
                                    std::shared_ptr<Object> obj);
};