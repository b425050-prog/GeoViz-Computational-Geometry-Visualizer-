/**
 * @file Commands.cpp
 * @brief Reversible scene edits implemented with exclusive smart-pointer ownership.
 */

#include "geoviz/app/Commands.hpp"

#include <utility>

namespace geoviz::app {

void AddPointCommand::execute(SceneModel& scene) {
    index_ = scene.addPoint(point_);
}

void AddPointCommand::undo(SceneModel& scene) {
    static_cast<void>(scene.removePoint(index_));
}

void RemovePointCommand::execute(SceneModel& scene) {
    removed_ = scene.removePoint(index_);
}

void RemovePointCommand::undo(SceneModel& scene) {
    scene.insertPoint(index_, removed_);
}

void MovePointCommand::execute(SceneModel& scene) {
    scene.setPoint(index_, to_);
}

void MovePointCommand::undo(SceneModel& scene) {
    scene.setPoint(index_, from_);
}

void ReplaceSceneCommand::execute(SceneModel& scene) {
    previous_ = scene.snapshot();
    scene.replace(replacement_);
}

void ReplaceSceneCommand::undo(SceneModel& scene) {
    scene.replace(previous_);
}

void CommandManager::execute(std::unique_ptr<SceneCommand> command, SceneModel& scene) {
    if (!command) return;
    command->execute(scene);
    undoStack_.push_back(std::move(command));
    redoStack_.clear();
    if (undoStack_.size() > historyLimit_) {
        undoStack_.erase(undoStack_.begin());
    }
}

bool CommandManager::undo(SceneModel& scene) {
    if (undoStack_.empty()) return false;
    std::unique_ptr<SceneCommand> command = std::move(undoStack_.back());
    undoStack_.pop_back();
    command->undo(scene);
    redoStack_.push_back(std::move(command));
    return true;
}

bool CommandManager::redo(SceneModel& scene) {
    if (redoStack_.empty()) return false;
    std::unique_ptr<SceneCommand> command = std::move(redoStack_.back());
    redoStack_.pop_back();
    command->execute(scene);
    undoStack_.push_back(std::move(command));
    return true;
}

void CommandManager::clear() noexcept {
    undoStack_.clear();
    redoStack_.clear();
}

}  // namespace geoviz::app

