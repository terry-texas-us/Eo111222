Current major task: substantial changes to the .peg file for easier linear DXF parsing using handle architecture (header/tables/entities).

When user asks for big-picture evaluation or development plan:
1. First explore the entire codebase (use @folder or list files as needed).
2. Focus especially on Application/AeSys, all EoDb* primitives, .peg serialization, and current DXF import/export.
3. Provide a structured, phased development plan that coordinates with SuperGrok (Grok 4.20) and VS 2026 workflow.
4. Output plans in clear sections: Current State, Proposed Architecture, Step-by-Step Implementation (with exact Visual Studio actions), Test Strategy, File Format Compatibility Guarantees.
5. Always suggest code that I (Terry) can review in VS 2026 + GitHub Copilot + SuperGrok before committing.