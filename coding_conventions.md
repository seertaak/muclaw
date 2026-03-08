# NanoClaw Coding Conventions

## Definitions

1. A function is a subroutine which returns a value.
2. A pure function is a function which doesn't modify its arguments.
3. A predicate is a pure function returning `bool`.
4. A procedure is a subroutine which modifies its arguments.

## Naming
- **Classes**: **DO** use `PascalCase`
- **Functions and Predicates**: 
    - **MUST** return a non-void value
    - **MUST NOT** modify their arguments
    - **MUST** use `snake_case`
    - **MUST** use trailing return types. 
        ```
        auto length(const vec3& v) -> double {
            return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
        }
        ```
    - Predicate names **MUST** be adjectives describing the property being tested (e.g., `if (sorted(xs)) ...`).
    - Function names **MUST** be nouns describing the value returned (e.g., `auto n = size(xs);` or `auto z = product(x, y);`).
- **Procedures**: 
    - Procedure names **MUST** be verbs describing the action being performed (e.g., `sort(xs);`).
    - **MUST** use `snake_case`
- **Getters and Setters**: We don't use `get_` or `set_` prefixes; use the property name instead. Getters follow the noun rule for functions. Setters are technically procedures, but as a special case, they break the verb convention and simply use the property name.
- **Variables**: 
    - **SHOULD** use `snake_case`, **UNLESS** the variable represents a matrix or tensor, in which case its name **SHOULD** follow mathematical or scientific conventions.
    - **DO** use the shortest possible name which avoids ambiguity. 
        ```
        // Single proxy in scope
        {
            LLMProxy proxy;

            // use proxy
            ...
        }
        // 2 proxies used in the same scope
        {
            LLMProxy llm_proxy;
            TelegramProxy telegram_proxy;

            // use llm_proxy and telegram_proxy
            ...
        }
        ```
    - Variable shadowing **MAY** be used to keep names short. 
    - Local variables **MAY** be abbreviated for commonly used terms.
        - for commonly-used terms (eg. `db` not `database`, `ModelRef` not `ModelReference`, `SmartPtr` not `SmartPointer`)
    - Member Variables **MUST** use trailing suffixes.

## Formatting
- **Indentation**: 4 spaces.
- **Braces**: 
    1. Opening braces for `if`/`for`/functions/etc. go on the same line as the statement.
    2. When a block in an `if` or `for` statement is a single line, braces **MUST** be elided.
- **Blank Lines**: Use blank lines to separate logical blocks of code.

## Comments
- **DO NOT** write comments that describe what the following line or short block of code is doing.
- **DO** generate a comment when the code that follows is surprising or unintuitive.  
- Comments **MAY** used to signal that some piece of code is subject to change.
