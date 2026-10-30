#pragma once

inline bool modifier(const std::set<int> &keys, int left, int right) {
    return IsKeyDown(left) || IsKeyDown(right) || IsKeyReleased(left) || IsKeyReleased(right) ||
           keys.count(left) || keys.count(right);
}
inline void editText(std::string &value, TextCursor &cursor, const std::set<int> &keys,
                     std::size_t limit) {
    const bool ctrl = modifier(keys, KEY_LEFT_CONTROL, KEY_RIGHT_CONTROL);
    const bool shift = modifier(keys, KEY_LEFT_SHIFT, KEY_RIGHT_SHIFT);
    if (ctrl && keys.count(KEY_A))
        cursor.selectAll(value);
    if (ctrl && (keys.count(KEY_C) || keys.count(KEY_X)) && cursor.selected()) {
        SetClipboardText(cursor.selectedText(value).c_str());
        if (keys.count(KEY_X))
            cursor.replace(value, "", limit);
    }
    if (ctrl && keys.count(KEY_V)) {
        if (const char *clip = GetClipboardText())
            cursor.replace(value, clip, limit);
    }
    if (keys.count(KEY_LEFT))
        cursor.left(value, shift);
    if (keys.count(KEY_RIGHT))
        cursor.right(value, shift);
    if (keys.count(KEY_HOME))
        cursor.move(0, shift);
    if (keys.count(KEY_END))
        cursor.move(value.size(), shift);
    if (keys.count(KEY_BACKSPACE))
        cursor.backspace(value);
    if (keys.count(KEY_DELETE))
        cursor.erase(value);
    for (int ch = GetCharPressed(); ch; ch = GetCharPressed()) {
        if (!ctrl && ch >= 32 && ch != 127) {
            int size = 0;
            const char *bytes = CodepointToUTF8(ch, &size);
            cursor.replace(value, std::string(bytes, static_cast<std::size_t>(size)), limit);
        }
    }
}
inline void drawEditor(const std::string &value, const TextCursor &cursor, Rectangle area,
                       int size = 17, bool multiline = false, bool focused = true) {
    std::vector<std::pair<std::size_t, std::size_t>> lines;
    std::size_t start = 0, i = 0, space = std::string::npos;
    while (multiline && i < value.size()) {
        const auto end = TextCursor::next(value, i);
        if (value[i] == '\n') {
            lines.push_back({start, i});
            start = end;
            space = std::string::npos;
        } else if (Ui::measure(value.substr(start, end - start).c_str(), size) > area.width &&
                   i > start) {
            const auto split = space != std::string::npos && space >= start ? space + 1 : i;
            lines.push_back({start, split});
            start = i = split;
            space = std::string::npos;
            continue;
        } else if (value[i] == ' ')
            space = i;
        i = end;
    }
    lines.push_back({start, value.size()});
    std::size_t caretLine = 0;
    while (caretLine + 1 < lines.size() && cursor.position() >= lines[caretLine + 1].first)
        ++caretLine;
    const float height = Ui::fontSize(size) + 6;
    const auto visible = static_cast<std::size_t>(std::max(1.0f, std::floor(area.height / height)));
    const auto firstLine = caretLine >= visible ? caretLine - visible + 1 : 0;
    const auto [selectionStart, selectionEnd] = cursor.selection();
    Ui::clip(area.x, area.y, area.width, area.height);
    for (std::size_t line = firstLine; line < std::min(lines.size(), firstLine + visible); ++line) {
        const auto [first, last] = lines[line];
        const auto widthTo = [&](std::size_t end) {
            return Ui::measure(value.substr(first, end - first).c_str(), size);
        };
        const float offset =
            multiline ? 0 : std::max(0.0f, widthTo(cursor.position()) - area.width + 8);
        const float x = area.x - offset, y = area.y + (line - firstLine) * height;
        const auto a = std::max(first, selectionStart), b = std::min(last, selectionEnd);
        if (a < b)
            DrawRectangleRec({x + widthTo(a), y, widthTo(b) - widthTo(a), height},
                             {174, 209, 183, 255});
        Ui::draw(value.substr(first, last - first), x, y, size, INK);
        if (line == caretLine && focused)
            DrawRectangleRec({x + widthTo(cursor.position()), y, 1.5f, Ui::fontSize(size)}, ACCENT);
    }
    EndScissorMode();
}
