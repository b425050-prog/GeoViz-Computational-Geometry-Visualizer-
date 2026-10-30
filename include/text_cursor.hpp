#pragma once
#include <algorithm>
#include <string>
#include <utility>

namespace gv {
// Shared editing semantics for existing settings fields and review notes.
class TextCursor final {
    std::size_t position_ = 0, anchor_ = 0;

  public:
    static std::size_t previous(const std::string &value, std::size_t index) {
        index = std::min(index, value.size());
        if (index)
            --index;
        while (index && (static_cast<unsigned char>(value[index]) & 0xc0) == 0x80)
            --index;
        return index;
    }
    static std::size_t next(const std::string &value, std::size_t index) {
        index = std::min(index + 1, value.size());
        while (index < value.size() && (static_cast<unsigned char>(value[index]) & 0xc0) == 0x80)
            ++index;
        return index;
    }
    std::size_t position() const { return position_; }
    std::pair<std::size_t, std::size_t> selection() const {
        return std::minmax(position_, anchor_);
    }
    bool selected() const { return position_ != anchor_; }
    void reset(std::size_t position = 0) { position_ = anchor_ = position; }
    void move(std::size_t position, bool extend = false) {
        position_ = position;
        if (!extend)
            anchor_ = position;
    }
    void selectAll(const std::string &value) {
        anchor_ = 0;
        position_ = value.size();
    }
    void left(const std::string &value, bool extend = false) {
        move(selected() && !extend ? selection().first : previous(value, position_), extend);
    }
    void right(const std::string &value, bool extend = false) {
        move(selected() && !extend ? selection().second : next(value, position_), extend);
    }
    std::string selectedText(const std::string &value) const {
        const auto [first, last] = selection();
        return value.substr(first, last - first);
    }
    bool replace(std::string &value, const std::string &input, std::size_t limit) {
        const auto [first, last] = selection();
        if (last > value.size() || value.size() - (last - first) + input.size() > limit)
            return false;
        value.replace(first, last - first, input);
        reset(first + input.size());
        return true;
    }
    void backspace(std::string &value) {
        if (!selected())
            anchor_ = previous(value, position_);
        replace(value, "", value.size());
    }
    void erase(std::string &value) {
        if (!selected())
            anchor_ = next(value, position_);
        replace(value, "", value.size());
    }
};
} // namespace gv
