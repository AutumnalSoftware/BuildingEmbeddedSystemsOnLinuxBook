# Designing Deterministic Systems with Modern C++

_(Formerly **Building Embedded Systems on Linux: A Modern C++ Approach**.)_

## About This Repository

This repository contains companion code for  
**Designing Deterministic Systems with Modern C++: Host-First Development on Linux**  
https://leanpub.com/buildingembeddedsystemsonlinuxamoderncapproach

Each chapter directory corresponds directly to code discussed in the book, covering topics such as:

* modern C++ type erasure
* deterministic pipelines
* serialization and messaging
* system composition and control
* host-based embedded development
* and real-world system architecture patterns

A free sample chapter is available at the link above.

The code here is intended to support **released chapters** of the book. During writing, I may push intermediate or experimental code that does not yet correspond directly to published text. When that happens, the book should be considered the authoritative reference.

Code will be updated and refined as new chapters are released.

> **Note:** This repository may contain work-in-progress code ahead of the current published chapters.

---
## Building the Code
```
cmake -S . -B build
cmake --build build
```

This will build the entire tree.

## Feedback, Questions, and Discussion

If you clone this repository and later find yourself wondering *why something is designed a certain way*, or if a section of the code or book felt unclear, **that feedback is valuable** — even if it’s months later and even if it’s not fully formed yet.

The easiest way to ask questions or leave feedback is to open a GitHub issue (templates are provided).  
If you’d prefer not to use GitHub, you can also email me directly at **mark@autumnalsoftware.com**.

Both technical and conceptual feedback are welcome.

Thank you for reading — and for taking the time to dig into the code.
