<!--
Copyright Glen Knowles 2022 - 2026.
Distributed under the Boost Software License, Version 1.0.
-->

# Release Checklist
1. Be on the 6.2.x branch.
2. Commit or stash all modified files.
3. Verify CMakeDeps.cmake is current.
    - cd build
    - cmake ..
    - Commit CMakeDeps.cmake if changed
4. Verify tests pass.
    - cd bin
    - cli --test
5. Verify code samples in the guide work as described.
    - docgen test
6. Change "Unreleased" section in CHANGELOG.md to the new version.
7. Add new version to docs\docgen.xml.
8. Verify copyright dates are current.
    - cmtupd -u
    - Commit updated files
9. Push to github with description of "Update version to X.Y.Z"
10. Wait for CI to succeed.
11. Draft a new release on github.
    1. Go to Releases, click on "Draft a new release"
    2. Change target pull down to 6.2.x
    3. Select "Select tag" and create the new tag "v{major}.{minor}.{patch}".
    4. Set title to the same as the new tag.
    5. Write brief description and publish the release.
12. Publish the docs
    1. Run "git pull" to get the new tag created in 11.4.
    2. Run "docgen site".
    3. Go to vendor\gh-pages directory.
    4. Review, commit, and push the new docs with description of "Update docs".
