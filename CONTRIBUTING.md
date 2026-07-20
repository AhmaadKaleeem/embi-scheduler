# Contributing to EMBI Scheduler

Thank you for your interest in contributing to the EMBI Scheduler research project!

## Development Workflow

1. **Fork and Clone**: Fork the repository and clone it locally.
2. **Branch**: Create a new feature branch (`git checkout -b feature/your-feature`).
3. **Build and Test**: Ensure the C++ simulator builds and all tests pass:
   ```bash
   cd Event-driven-simulator-C++
   cmake -B build
   cmake --build build
   cd build && ctest
   ```
4. **Commit**: Write clear, descriptive commit messages.
5. **Push and PR**: Push to your fork and submit a Pull Request.

## Bug Reports and Feature Requests
Please use GitHub Issues to report bugs or request features. Include detailed steps to reproduce for any bugs found in the simulator.

## Code Style
- **C++**: We use `clang-format`. Please format your code before submitting a PR.
- **Python**: Follow PEP 8 guidelines.

## Academic Integrity
As this is an active research project, any external algorithms or heuristics added to the simulator must be properly cited with their original academic sources in the PR description and code comments.
