#pragma once
#include <memory>
#include <vector>

#include "EoGeTransformMatrix.h"

class AeSysDoc;
class EoDbGroup;
class EoDbLayer;
class EoDbPrimitive;

/// @brief Abstract base for all undoable document commands (Phase 3A).
///
/// Each concrete command owns the data needed to undo and redo itself.
/// The Execute/Undo contract:
///   - Execute: perform the operation and record enough state to reverse it.
///   - Undo:    reverse the operation exactly.
///   - Redo:    re-apply the operation (default: calls Execute again).
///
/// Commands are stored in EoDocCommandStack on AeSysDoc and never outlive
/// the document.  AeSysDoc::DeleteContents must call commandStack.Clear().
class EoDocCommand {
 public:
  EoDocCommand() = default;
  virtual ~EoDocCommand() = default;
  EoDocCommand(const EoDocCommand&) = delete;
  EoDocCommand& operator=(const EoDocCommand&) = delete;
  EoDocCommand(EoDocCommand&&) = delete;
  EoDocCommand& operator=(EoDocCommand&&) = delete;

  /// Human-readable label shown in the Undo/Redo menu item tooltip.
  [[nodiscard]] virtual const wchar_t* Label() const noexcept = 0;

  /// Perform (or re-perform) the operation.
  virtual void Execute(AeSysDoc* doc) = 0;

  /// Reverse the operation.
  virtual void Undo(AeSysDoc* doc) = 0;

  /// Called just before the command is permanently discarded (evicted from the undo cap
  /// or cleared by DeleteContents).  Override to unregister handles or release
  /// other document-side resources owned by this command.
  /// Default: no-op (safe for commands that hold no document resources).
  virtual void Discard(AeSysDoc* /*doc*/) noexcept {}
};

/// @brief Fixed-capacity undo/redo stack living on AeSysDoc (Phase 3A).
///
/// Pushing a new command clears the redo branch — standard linear undo model.
/// The stack owns all commands via unique_ptr.
class EoDocCommandStack {
 public:
  static constexpr std::size_t kMaxDepth{64};

  EoDocCommandStack() = default;
  EoDocCommandStack(const EoDocCommandStack&) = delete;
  EoDocCommandStack& operator=(const EoDocCommandStack&) = delete;

  /// Push an already-executed command onto the undo stack.
  /// Clears the redo branch.  Oldest entry is discarded when depth exceeds kMaxDepth.
  void Push(AeSysDoc* doc, std::unique_ptr<EoDocCommand> cmd) {
    for (auto& c : m_redo) { c->Discard(doc); }
    m_redo.clear();
    m_undo.push_back(std::move(cmd));
    if (m_undo.size() > kMaxDepth) {
      m_undo.front()->Discard(doc);
      m_undo.erase(m_undo.begin());
    }
  }

  [[nodiscard]] bool CanUndo() const noexcept { return !m_undo.empty(); }
  [[nodiscard]] bool CanRedo() const noexcept { return !m_redo.empty(); }

  [[nodiscard]] const wchar_t* UndoLabel() const noexcept {
    return CanUndo() ? m_undo.back()->Label() : L"";
  }
  [[nodiscard]] const wchar_t* RedoLabel() const noexcept {
    return CanRedo() ? m_redo.back()->Label() : L"";
  }

  void Undo(AeSysDoc* doc) {
    if (!CanUndo()) { return; }
    auto cmd = std::move(m_undo.back());
    m_undo.pop_back();
    cmd->Undo(doc);
    m_redo.push_back(std::move(cmd));
  }

  void Redo(AeSysDoc* doc) {
    if (!CanRedo()) { return; }
    auto cmd = std::move(m_redo.back());
    m_redo.pop_back();
    cmd->Execute(doc);
    m_undo.push_back(std::move(cmd));
  }

  /// Discard all commands, releasing document-side resources.
  /// Must be called from AeSysDoc::DeleteContents before destroying document objects.
  void Clear(AeSysDoc* doc) noexcept {
    for (auto& c : m_undo) { c->Discard(doc); }
    for (auto& c : m_redo) { c->Discard(doc); }
    m_undo.clear();
    m_redo.clear();
  }

 private:
  std::vector<std::unique_ptr<EoDocCommand>> m_undo{};
  std::vector<std::unique_ptr<EoDocCommand>> m_redo{};
};

/// @brief Undoable delete-group command (Phase 3B).
///
/// Execute: removes the group from its layer and all views (already done by the
///          caller before Push — ownership transferred here).
/// Undo:    re-adds the group to the original layer and all views.
/// Redo:    removes the group again (re-executes the delete).
///
/// Ownership is managed via unique_ptr:
///   - On the redo branch (after Execute): m_ownedGroup holds the group.
///   - On the undo branch (after Undo):    m_ownedGroup is empty; document owns the group.
class EoDocCmdDeleteGroup final : public EoDocCommand {
 public:
  /// Construct after the delete has already been performed.
  /// @param group  The deleted group (ownership transferred to this command).
  /// @param layer  The layer the group was removed from (non-owning).
  EoDocCmdDeleteGroup(EoDbGroup* group, EoDbLayer* layer) noexcept
      : m_ownedGroup{group}, m_group{group}, m_layer{layer} {}

  [[nodiscard]] const wchar_t* Label() const noexcept override { return L"Delete Group"; }

  /// Re-delete: remove from layer/views, take ownership back.
  void Execute(AeSysDoc* doc) override;

  /// Restore: add back to original layer/views, release ownership to document.
  void Undo(AeSysDoc* doc) override;

 private:
  std::unique_ptr<EoDbGroup> m_ownedGroup{};  ///< Owns the group while on the redo branch.
  EoDbGroup* m_group{nullptr};                ///< Non-owning alias — valid for the command lifetime.
  EoDbLayer* m_layer{nullptr};                ///< Original layer — non-owning.

  void Discard(AeSysDoc* doc) noexcept override;  ///< Unregister handles when group is permanently evicted.
};

/// @brief Undoable delete-last-group command (Phase 3B — Backspace).
///
/// Same ownership model as EoDocCmdDeleteGroup.  The layer is captured from
/// AnyLayerRemove at delete time so Undo can restore to the exact original layer.
class EoDocCmdDeleteLastGroup final : public EoDocCommand {
 public:
  EoDocCmdDeleteLastGroup(EoDbGroup* group, EoDbLayer* layer) noexcept
      : m_ownedGroup{group}, m_group{group}, m_layer{layer} {}

  [[nodiscard]] const wchar_t* Label() const noexcept override { return L"Delete Last Group"; }

  void Execute(AeSysDoc* doc) override;
  void Undo(AeSysDoc* doc) override;
  void Discard(AeSysDoc* doc) noexcept override;

 private:
  std::unique_ptr<EoDbGroup> m_ownedGroup{};
  EoDbGroup* m_group{nullptr};
  EoDbLayer* m_layer{nullptr};
};

/// @brief Undoable trap-cut command (Phase 3C).
///
/// Captures all currently trapped groups with their origin layers, removes them
/// from the document (same visual effect as DeleteAllTrappedGroups), and keeps
/// them alive for Undo.  The clipboard copy is performed by the caller before
/// constructing this command — this command only owns the delete/restore lifecycle.
///
/// Ownership model: same unique_ptr pattern as EoDocCmdDeleteGroup.
///   - Redo branch: entry.owned holds the group.
///   - Undo branch: group lives in its origin layer; owned is empty.
class EoDocCmdTrapCut final : public EoDocCommand {
 public:
  struct Entry {
    EoDbGroup* group{nullptr};           ///< Non-owning alias — always valid.
    EoDbLayer* layer{nullptr};           ///< Origin layer — non-owning.
    std::unique_ptr<EoDbGroup> owned{};  ///< Owns the group while on the redo branch.
  };

  /// Construct with already-removed groups.  Each entry must have group+layer set;
  /// owned starts empty — caller transfers ownership by setting owned = unique_ptr(group)
  /// after removing from the layer.
  explicit EoDocCmdTrapCut(std::vector<Entry> entries) noexcept
      : m_entries{std::move(entries)} {}

  [[nodiscard]] const wchar_t* Label() const noexcept override { return L"Cut"; }

  void Execute(AeSysDoc* doc) override;
  void Undo(AeSysDoc* doc) override;
  void Discard(AeSysDoc* doc) noexcept override;

 private:
  std::vector<Entry> m_entries{};
};

/// @brief Undoable primitive-delete command (Phase 3D).
///
/// Two sub-cases depending on whether the primitive was the sole occupant of
/// its group:
///
///   Solo (group had 1 primitive):
///     The entire group is removed from its layer.  Undo re-adds it.
///     m_sourceGroup is nullptr; m_wrapperGroup owns the (unchanged) group.
///
///   Multi (group had >1 primitives):
///     The primitive is removed from m_sourceGroup and wrapped in a new
///     single-primitive group.  Undo re-inserts the primitive into
///     m_sourceGroup and destroys the wrapper.
///     m_sourceGroup is non-owning (still in the document);
///     m_wrapperGroup owns the extracted primitive's wrapper group.
class EoDocCmdDeletePrimitive final : public EoDocCommand {
 public:
  /// Solo constructor — whole group removed from layer.
  EoDocCmdDeletePrimitive(EoDbGroup* group, EoDbLayer* layer) noexcept
      : m_wrapperGroup{group}, m_rawWrapper{group}, m_layer{layer},
        m_sourceGroup{nullptr}, m_primitive{nullptr} {}

  /// Multi constructor — primitive extracted into wrapper group.
  EoDocCmdDeletePrimitive(EoDbGroup* wrapper, EoDbGroup* sourceGroup, EoDbPrimitive* primitive) noexcept
      : m_wrapperGroup{wrapper}, m_rawWrapper{wrapper}, m_layer{nullptr},
        m_sourceGroup{sourceGroup}, m_primitive{primitive} {}

  [[nodiscard]] const wchar_t* Label() const noexcept override { return L"Delete Primitive"; }

  void Execute(AeSysDoc* doc) override;
  void Undo(AeSysDoc* doc) override;
  void Discard(AeSysDoc* doc) noexcept override;

 private:
  std::unique_ptr<EoDbGroup> m_wrapperGroup{};  ///< Owns the solo group (redo) or the empty wrapper (always, multi).
  EoDbGroup* m_rawWrapper{nullptr};              ///< Non-owning alias.
  EoDbLayer* m_layer{nullptr};                   ///< Origin layer (solo only; nullptr for multi).
  EoDbGroup* m_sourceGroup{nullptr};             ///< nullptr = solo; non-null = multi.
  EoDbPrimitive* m_primitive{nullptr};           ///< The extracted primitive (multi only).
};

/// @brief Undoable add-group command (Phase 3E).
///
/// Wraps every interactive group creation (AddWorkLayerGroup choke point).
/// The command is constructed and pushed AFTER the group has already been added
/// to the work layer and all views — matching the delete-command convention.
///
/// Ownership model (inverse of EoDocCmdDeleteGroup):
///   - Undo branch (after Undo): m_ownedGroup holds the group (removed from document).
///   - Redo branch (after Redo / initial Push): m_ownedGroup is empty; document owns the group.
class EoDocCmdAddGroup final : public EoDocCommand {
 public:
  /// Construct after the group has already been added to the work layer.
  /// @param group  The newly added group (non-owning on construction — document owns it).
  /// @param layer  The work layer the group was added to (non-owning).
  EoDocCmdAddGroup(EoDbGroup* group, EoDbLayer* layer) noexcept
      : m_group{group}, m_layer{layer} {}

  [[nodiscard]] const wchar_t* Label() const noexcept override { return L"Draw"; }

  /// Redo: re-add the group to the layer and all views, release ownership to document.
  void Execute(AeSysDoc* doc) override;

  /// Undo: remove from layer and all views, take ownership.
  void Undo(AeSysDoc* doc) override;

  /// Discard: unregister handles when permanently evicted.
  void Discard(AeSysDoc* doc) noexcept override;

 private:
  EoDbGroup* m_group{nullptr};                ///< Non-owning alias — always valid.
  EoDbLayer* m_layer{nullptr};                ///< Work layer at creation time — non-owning.
  std::unique_ptr<EoDbGroup> m_ownedGroup{};  ///< Owns the group while on the undo branch.
};

/// @brief Undoable transform command for trapped groups (Phase 3F).
///
/// Covers all trap-level geometric operations: move, rotate, scale, flip, mirror.
/// Groups remain in the document throughout — this command holds only non-owning
/// pointers and the forward transform matrix.  Undo applies the inverse; Redo
/// re-applies the forward.  No Discard override needed (no owned heap objects).
///
/// Construction: snapshot the trapped group pointers and the forward matrix
/// BEFORE calling TransformTrappedGroups / TranslateTrappedGroups.
class EoDocCmdTransformGroups final : public EoDocCommand {
 public:
  /// @param groups    Non-owning snapshot of the trapped groups at transform time.
  /// @param matrix    The forward transform that was applied.
  /// @param label     Human-readable label (e.g. L"Move", L"Rotate", L"Scale").
  EoDocCmdTransformGroups(std::vector<EoDbGroup*> groups,
      const EoGeTransformMatrix& matrix,
      const wchar_t* label)
      : m_groups{std::move(groups)}, m_forwardMatrix{matrix}, m_inverseMatrix{matrix}, m_label{label} {
    m_inverseMatrix.Inverse();
  }

  [[nodiscard]] const wchar_t* Label() const noexcept override { return m_label; }

  /// Redo: re-apply the forward transform.
  void Execute(AeSysDoc* doc) override;

  /// Undo: apply the inverse transform.
  void Undo(AeSysDoc* doc) override;

 private:
  std::vector<EoDbGroup*> m_groups{};   ///< Non-owning — groups live in the document.
  EoGeTransformMatrix m_forwardMatrix{};
  EoGeTransformMatrix m_inverseMatrix{};  ///< Pre-computed at construction; avoids inverting a potentially singular matrix at undo time.
  const wchar_t* m_label{L"Transform"};
};