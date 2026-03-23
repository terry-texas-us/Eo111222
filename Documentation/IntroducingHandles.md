Certainly. Here is a detailed plan to refactor the `EoDbPrimitive` hierarchy to support a handle-based architecture, as described in your project instructions, while preserving all required contracts and compatibility.

---

## 1. **Goals & Constraints**

- **Introduce Handles:** Each primitive (and relevant objects) should have a unique handle (likely a 64-bit integer or similar) for referencing, as per DXF and future PEG format needs.
- **Preserve Virtual Contract:** All virtuals in `EoDbPrimitive` and derived classes must be preserved (signatures, ABI).
- **On-Disk Format:** No breaking changes to the PEG file format unless explicitly planned for the new handle fields.
- **MFC Document/View:** All changes must remain compatible with the MFC document/view architecture.
- **.clang-format & .clang-tidy:** Follow repository formatting and static analysis rules.
- **Minimize Raw Pointers:** Where possible, use smart pointers for new code, but do not break existing pointer-based APIs.
- **Consistent Naming:** Use camelCase for locals, PascalCase for types, and mark simple getters as `noexcept` and `[[nodiscard]]` where possible.

---

## 2. **Handle Architecture Design**

- **Handle Generation:** Centralize handle generation (e.g., static counter or UUID if needed).
- **Primitive Handles:** Each `EoDbPrimitive` and derived object gets a unique handle.
- **Reference Handles:** Where a primitive references another object (e.g., header, table, group), store the handle, not a raw pointer.
- **PEG Format:** Extend serialization/deserialization to read/write handles, but preserve legacy compatibility (e.g., add handle as an optional field).

---

## 3. **Class Hierarchy Refactor Plan**

### a. **EoDbPrimitive (Base Class)**
- Add a private/protected `std::uint64_t m_handle;` member.
- Add public `[[nodiscard]] std::uint64_t Handle() const noexcept;` getter.
- Add protected setter for handle (for deserialization).
- Update constructors to assign/generate a handle.
- Update serialization/deserialization to include handle (optional for backward compatibility).
- Update equality and hashing if needed to use handle.

### b. **Derived Classes (EoDbLine, EoDbPoint, EoDbPolyline, etc.)**
- Inherit handle logic from base.
- If referencing other objects (e.g., group, header), store their handles.
- Update serialization/deserialization for new handle fields.

### c. **Groups, Tables, Headers**
- If not already present, add handle fields to these classes.
- Ensure all references between primitives and these objects use handles.

---

## 4. **PEG File Format Changes**

- **Backward Compatibility:** If possible, make handle fields optional or add a version marker.
- **Serialization:** On write, output handle fields after legacy data.
- **Deserialization:** On read, if handle field is present, use it; otherwise, generate a new handle.

---

## 5. **Document/View Integration**

- Ensure all document/view operations (selection, update, etc.) use handles for referencing primitives where appropriate.
- Update any code that traverses or manages primitives to use handles for lookup and reference.

---

## 6. **Testing & Validation**

- Add unit tests for handle uniqueness, serialization, and deserialization.
- Test round-trip (write/read) for PEG files with and without handles.
- Validate that all document/view operations remain functional.

---

## 7. **Modernization Opportunities**

- Where possible, replace raw `new`/`delete` with `std::unique_ptr` or `std::shared_ptr` (without breaking MFC or serialization).
- Mark simple geometric operations and getters as `noexcept` and `[[nodiscard]]`.
- Use `Eo::geometricTolerance` and `Eo::numericEpsilon` for comparisons.

---

## 8. **Migration Steps**

1. Define `std::uint64_t` handle generation utilities.
2. Refactor `EoDbPrimitive` to include handle logic.
3. Update all derived classes to support handle architecture.
4. Update serialization/deserialization for PEG format.
5. Refactor group/table/header classes to use handles.
6. Update document/view and group management code.
7. Add/Update tests.
8. Review and modernize code per guidelines.

---