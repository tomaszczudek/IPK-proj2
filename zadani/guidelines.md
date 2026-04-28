# IPK-Project-Guidelines
Welcome to the IPK projects guideline repository.

Contact Person: pluskal@vut.cz

> [!WARNING]
> All communication regarding project assignments SHOULD be held in issues in the respective project repositories.
> Questions regarding project assignments SHOULD NOT be asked via Moodle forum or email.

## Table of Contents

- [IPK-Project-Guidelines](#ipk-project-guidelines)
  - [Table of Contents](#table-of-contents)
  - [Interpretation Rules](#interpretation-rules)
  - [Goal](#goal)
  - [Coding Instructions](#coding-instructions)
  - [Submission Instructions](#submission-instructions)
  - [README.md Expectations](#readmemd-expectations)
  - [Developer Environment and Virtualization Remarks](#developer-environment-and-virtualization-remarks)
  - [Evaluation](#evaluation)
  - [Project References](#project-references)
  - [Credits](#credits)

## Interpretation Rules
To avoid ambiguity, this document uses:
* `MUST` / `MUST NOT` for mandatory requirements.
* `SHOULD` / `SHOULD NOT` for strong recommendations.
* `MAY` for optional behavior.

## Goal
Implement a network application that satisfies the selected project assignment.

If the assignment does not explicitly define behavior, you MAY choose the behavior yourself, but you MUST document that decision in `README.md`.

## Coding Instructions
Supported languages:
* C (C17)
* C++ (C++20)
* C#

All implementations MUST run on the reference virtual machine with the provided developer environment.

Language-specific requirement:
* Write idiomatic code for the selected language.
* Follow the language's native programming paradigm, e.g., for C++ and C#, OOP MUST be used.

Build requirements:
* A root-level `Makefile` MUST be provided.
* Evaluators WILL run `make NixDevShellName` and `make`.
* `make NixDevShellName` MUST print exact name of Nix `devShell` required for build and execution of the student project.
  For example, [`c` for C/C++](https://git.fit.vutbr.cz/NESFIT/dev-envs/src/branch/main/nix/devShells/c.nix) or
  [`csharp` for C#](https://git.fit.vutbr.cz/NESFIT/dev-envs/src/branch/main/nix/devShells/csharp.nix). Stripped string
  from `stdout` is used for activation of Nix development environment.
* Evaluators WILL evaluate build inside the `devShell` returned by `make NixDevShellName`.
* Evaluators WILL execute the application in the same `devShell`.
* The project MUST build and run correctly in that `devShell`.
* Self-contained executables SHOULD be provided whenever feasible:
  * for C/C++, binaries SHOULD run without additional runtime setup;
  * for C#, self-contained publish SHOULD be used (`--self-contained true`, ideally with `-p:PublishSingleFile=true`);
  * for other assignment-permitted languages, a self-contained-like delivery SHOULD be used.
* `make` MUST build the required executable name defined by the assignment.
  * The executable MUST be located in root of the project/repository/archive.
  * The executable MUST be standalone _executable file_ (`chmod +x`).
  * For C#, `make` MAY call `dotnet publish` via an appropriate phony target.
* The `Makefile`, the compiled binary, or any of their parts MUST NOT implement custom software download logic from the internet.
* Downloads performed by standard language/build package managers (for example: NuGet for C#) MAY be used unless the assignment explicitly states otherwise.

Code quality and behavior requirements:
* Platform-dependent behavior MUST be explicitly marked (for example: Linux-only functionality).
* Source code MUST be structured, readable, and split into meaningful modules, methods, and functions.
* The program MUST provide help/usage output.
* Error messages MUST be understandable.
* The program output MUST be written to `stdout`, error messages MUST be written to `stderr`.
* All program outputs, project source code, and project documentation files MUST be valid text sequences.
* Exit code of the program MUST be `0` for successful execution and non-zero for failed execution. The program
  SHOULD use meaningful exit codes for various error types[^sysexits][^exit_codes].
* The program MUST NOT terminate due to avoidable runtime crashes (for example: `SEGMENTATION FAULT`, division by zero).
* The program MUST NOT hang for unreasonable amount of time. Tests use appropriate timeouts for program execution.
* If the program works with temporary files, all of them MUST be cleaned up upon program termination.

Student automated tests:
* Students MUST provide their own automated tests.
* The project `Makefile` MUST provide a `test` target (`make test`) that executes these tests.
* Student test sets SHOULD cover all functionality required by the assignment.
* Native testing framework(s) for the selected language SHOULD be used.
* If required test dependencies are missing in the default `devShell`, you MAY submit a PR to the `dev-envs` repository.
* `devShell` extension PR approval is not automatic; each extension request MUST be technically justified during review.


Unless the assignment explicitly states otherwise, all libraries MAY be used.

> <span style="color:orange">When versioning your project, you SHOULD use a **private** [Git](https://git-scm.com/) repository on the faculty [Gitea portal](https://git.fit.vutbr.cz/).
You SHOULD grant access to all [IPK instructors](https://git.fit.vutbr.cz/org/NESFIT/members).
</span>

Your repository MUST contain multiple commits representing your implementation history.
Projects SHOULD use [semantic commit messages](https://www.conventionalcommits.org/en/v1.0.0/).

## Submission Instructions
The project repository and submitted ZIP MUST contain at least the following files:

* all source code and any stand-alone libraries required to build and run;
* a working root-level `Makefile`;
* `README.md` in Markdown (see requirements below);
* `LICENSE` (a license of your choice);
* `CHANGELOG.md` with implemented functionality and known limitations.

If there are no known limitations, `CHANGELOG.md` MUST state that explicitly.

File placement rules:
* Mandatory metadata files (`Makefile`, `README.md`, `LICENSE`, `CHANGELOG.md`) MUST be in the repository root.
* Source code and libraries SHOULD be placed in language-appropriate subdirectories.
* Build artifacts, `.git` directory, temporary files, and nested archives MUST NOT be included in the submitted ZIP.

Submission checklist:

1) Verify that the project builds successfully in the <ins>reference developer environment</ins> and that mandatory filenames are correct.
2) Upload exactly one ZIP archive named `xlogin99.zip` containing your repository contents.
   If you do not have such login assigned in information system, use your personal number (i.e., "osobní
   číslo") to name your archive (e.g., `123456.zip` in case of `123456` personal number). Make sure the archive can be extracted using
   `unzip` in the <ins>reference developer environment</ins> without any errors or warnings.
3) Grant access to your private Gitea repository to all members of [NES@FIT Gitea group](https://git.fit.vutbr.cz/org/NESFIT/members).

> [!WARNING]
> Any submission sent after the deadline via email or any unofficial channel will be ignored.

## README.md Expectations
`README.md` MUST include at least:
* a short project overview;
* build and run instructions;
* a clear description of implemented features and behavior;
* explanation of important design decisions;
* a testing section with reproducible test procedure and results;
* known limitations (or an explicit statement that none are known);
* references/sources used.

`README.md` MUST be written in one of the following languages:
* Czech,
* Slovak,
* or English.

Source code comments MUST be in English.

Testing requirements in `README.md`:
* Tests MUST be reproducible (include environment details such as topology, hardware/software versions where relevant).
* Tests MUST cover normal behavior and edge cases.
* `README.md` MUST describe how to execute automated tests (e.g. `make test`).
* If a comparable tool exists, a brief comparison SHOULD be included.
* Test sets SHOULD be your own.
* Textual outputs SHOULD be preferred over screenshots for command-line results.
* Listing raw inputs/outputs alone is not enough. The testing section MUST state:
1. what was tested
2. why it was tested
3. how it was tested
4. what was the testing environment
5. what were the inputs, expected outputs, and actual outputs

When citing sources, you MUST follow [faculty citation guidelines](https://www.fit.vut.cz/study/theses/citations/.en).

When adopting or inserting snippets of someone else's source code into your project, you need to use them according to the author's license terms.
Moreover, you need to mark this source code and reference the author explicitly in your source files.
Avoid any unidirectional or bidirectional plagiarism!

## Developer Environment and Virtualization Remarks
The reference environment is located in [its own repository](https://git.fit.vutbr.cz/NESFIT/dev-envs).

> <span style="color:orange">Projects will be evaluated on the reference virtual machine (`x86_64-linux`).</span>

The following developer environments are currently prepared:
* `c`
* `csharp`

If the assignment permits a different language, you may create a PR following these [instructions](https://git.fit.vutbr.cz/NESFIT/dev-envs/src/branch/main/nix/devShells/README.md) and request approval.

To activate or modify the environment, follow the guide in the corresponding [developer environment repository](https://git.fit.vutbr.cz/NESFIT/dev-envs#starting-development-environment).
However, your `Makefile` MUST work with the default, unedited environment.

Due to the nature of the projects, you can assume that:
* the project will be run and evaluated with super-user privileges,
* the reference machine will be connected to the Internet.

## Evaluation

Evaluation considers the following criteria:
Evaluators WILL evaluate build and execution inside the `devShell` returned by `make NixDevShellName`.
The project MUST build and run correctly in that `devShell`.
Self-contained deliverables SHOULD be provided whenever feasible.

Code and application behavior:
* code MUST be structured and readable (avoid, for example, spaghetti code or monolithic single-file solutions)
* a functioning `Makefile` MUST be provided
* mandatory files MUST use correct names and placement
* submitted archives MUST NOT contain temporary or irrelevant files
* dynamic memory and resource handling MUST be correct (for example: no memory leaks, no unclosed file descriptors)
* program input/output MUST comply with the assignment specification

`README.md` quality:
* `README.md` MUST be present and sufficiently detailed
* citations and formatting MUST follow project and faculty guidelines
* testing evidence MUST be complete and credible

Submissions are considered unacceptable if:
* project cannot be compiled, executed, or tested
* program terminates abruptly (for example: `SEGFAULT`, `SIGSEGV`)
* solution is not compliant with the assignment
* plagiarism (follow-up disciplinary process applies)

The archive submitted via IS VUT will be the basis for your evaluation.
Files in the Gitea repo are additional content accompanying the submission but take no precedence over the content in IS VUT.
Therefore, any technical difficulties on the Gitea side will not affect your score.

----

## Project References
* [IPK-Project1](https://git.fit.vutbr.cz/NESFIT/IPK-Project1)
* [IPK-Project2](https://git.fit.vutbr.cz/NESFIT/IPK-Project2)

## Credits
This document was extracted from:
`https://git.fit.vutbr.cz/NESFIT/IPK-Projects-2024/src/branch/master/README.md`

Original authors (from the source repository history of `README.md`):
* Daniel Dolejška `<dolejska@fit.vut.cz>`
* Vladimir Vesely `<vesely@fit.vut.cz>`

---

[^sysexits]: https://man7.org/linux/man-pages/man3/sysexits.h.3head.html
[^exit_codes]: https://tldp.org/LDP/abs/html/exitcodes.html
