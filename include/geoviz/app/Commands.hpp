#pragma once

/**
 * @file Commands.hpp
 * @brief Undoable Command-pattern operations for editing the scene.
 */

#include "geoviz/app/SceneModel.hpp"

#include <cstddef>
#include <memory>
#include <string_view>
#include <vector>

namespace geoviz::app {

class SceneCommand {
public:
    virtual ~SceneCommand() = default;
    virtual void execute(SceneModel& scene) = 0;
    virtual void undo(SceneModel& scene) = 0;
    [[nodiscard]] virtual std::string_view description() const noexcept = 0;
};

class AddPointCommand final : public SceneCommand {
public:
    explicit AddPointCommand(Point point) : point_(std::move(point)) {}
    void execute(SceneModel& scene) override;
    void undo(SceneModel& scene) override;
    [[nodiscard]] std::string_view description() const noexcept override { return "Add point"; }

private:
    Point point_;
    std::size_t index_{0U};
};

class RemovePointCommand final : public SceneCommand {
public:
    explicit RemovePointCommand(std::size_t index) : index_(index) {}
    void execute(SceneModel& scene) override;
    void undo(SceneModel& scene) override;
    [[nodiscard]] std::string_view description() const noexcept override { return "Remove point"; }

private:
    std::size_t index_{0U};
    Point removed_;
};

class MovePointCommand final : public SceneCommand {
public:
    MovePointCommand(std::size_t index, Point from, Point to)
        : index_(index), from_(std::move(from)), to_(std::move(to)) {}
    void execute(SceneModel& scene) override;
    void undo(SceneModel& scene) override;
    [[nodiscard]] std::string_view description() const noexcept override { return "Move point"; }

private:
    std::size_t index_{0U};
    Point from_;
    Point to_;
};

class ReplaceSceneCommand final : public SceneCommand {
public:
    explicit ReplaceSceneCommand(SceneSnapshot replacement)
        : replacement_(std::move(replacement)) {}
    void execute(SceneModel& scene) override;
    void undo(SceneModel& scene) override;
    [[nodiscard]] std::string_view description() const noexcept override { return "Replace scene"; }

private:
    SceneSnapshot replacement_;
    SceneSnapshot previous_;
};

/** @brief Owns command history and provides bounded undo/redo stacks. */
class CommandManager final {
public:
    explicit CommandManager(std::size_t historyLimit = 100U) : historyLimit_(historyLimit) {}

    void execute(std::unique_ptr<SceneCommand> command, SceneModel& scene);
    [[nodiscard]] bool undo(SceneModel& scene);
    [[nodiscard]] bool redo(SceneModel& scene);
    void clear() noexcept;
    [[nodiscard]] bool canUndo() const noexcept { return !undoStack_.empty(); }
    [[nodiscard]] bool canRedo() const noexcept { return !redoStack_.empty(); }

private:
    std::size_t historyLimit_{100U};
    std::vector<std::unique_ptr<SceneCommand>> undoStack_;
    std::vector<std::unique_ptr<SceneCommand>> redoStack_;
};

}  // namespace geoviz::app

