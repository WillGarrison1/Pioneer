#ifndef OPTION_H
#define OPTION_H

#include <functional>
#include <ostream>
#include <string>

class Option
{
  public:
    virtual ~Option() {};

    inline const std::string& GetName() const
    {
        return name;
    }

    friend std::ostream& operator<<(std::ostream& os, const Option& option)
    {
        return os << option.AsString() << "\n";
    }

    virtual void Set(const std::string& input) = 0;

  protected:
    virtual std::string AsString() const = 0;

    std::string name;
};

class SpinOption : public Option
{
  public:
    SpinOption(std::string name, int min, int max, int def, std::function<void(int)> on_update)
        : min(min), max(max), def(def), cur(def), on_update(on_update)
    {
        this->name = name;
    }
    ~SpinOption() override = default;

    friend std::ostream& operator<<(std::ostream& os, const Option& option);

    void Set(const std::string& input) override;

    inline int GetValue()
    {
        return cur;
    }

  private:
    std::string AsString() const override
    {
        return "option name " + name + " type spin default " + std::to_string(def) + " min " + std::to_string(min) +
               " max " + std::to_string(max);
    }

    int min, max, def, cur;
    std::function<void(int)> on_update;
};

class CheckOption : public Option
{
  public:
    CheckOption(std::string name, bool def, std::function<void(bool)> on_update)
        : def(def), cur(def), on_update(on_update)
    {
        this->name = name;
    }

    ~CheckOption() override = default;

    void Set(const std::string& input) override;

    inline bool GetValue()
    {
        return cur;
    }

  private:
    std::string AsString() const override
    {
        return "option name " + name + " type check default " + std::to_string(def);
    }

    bool def, cur;
    std::function<void(bool)> on_update;
};

class StringOption : protected Option
{
  public:
    StringOption(std::string name, std::string def, std::function<void(std::string)> on_update)
        : def(def), cur(def), on_update(on_update)
    {
        this->name = name;
    }

    ~StringOption() override = default;

    friend std::ostream& operator<<(std::ostream& os, const Option& option);
    void Set(const std::string& input) override;

    inline std::string GetValue()
    {
        return cur;
    }

  private:
    std::string AsString() const override
    {
        return "option name " + name + " type string default " + def;
    }

    std::string def, cur;
    std::function<void(std::string)> on_update;
};

#endif