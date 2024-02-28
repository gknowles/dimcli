<!--
Copyright Glen Knowles 2022 - 2023.
Distributed under the Boost Software License, Version 1.0.
-->

# Release Checklist
1. Be on the dev branch.
2. Commit or stash all modified files.
3. Verify CMakeDeps.cmake is current.
    - cd build
    - cmake ..
    - Commit CMakeDeps.cmake if changed
4. Verify tests pass.
    - bin\cli --test
5. Verify code samples in the guide work as described.
    - docgen test
6. Change "Unreleased" section in CHANGELOG.md to the new version.
7. Add new version to docs\docgen.xml.
8. Verify copyright dates are current.
    - cmtupd -u
    - Commit updated files
9. Push to github.
10. Wait for CI to succeed.
11. Merge to master.
    1. git fetch origin master    # Fetch any possible changes from master.
    2. git merge master           # Merge any changes from master back to dev.
    3. git push origin dev:master # Update master head to same as dev.
12. Draft a new release on github.
    1. Go to Releases, click on "Draft a new release"
    2. Select "Choose a tag" and create the new tag "v{major}.{minor}.{patch}".
    3. Set title to the same as the new tag.
    4. Write brief description and publish the release.
13. Publish the docs
    1. Run "docgen site".
    2. Go to vendor\gh-pages directory.
    3. Review, commit, and push the new docs with description of "Update docs".
14. Publish to vcpkg
    1. Update version
        - version-semver and license in ports/dimcli/vcpkg.json
            1. Set version-semvar to "{major}.{minor}.{patch}"
            2. Set license to "BSL-1.0"
            3. Run "vcpkg format-manifest ports/dimcli/vcpkg.json"
        - REF and SHA512 in ports/dimcli/portfile.cmake
            1. Set REF to the new tag
            2. Run "vcpkg install dimcli", it will fail and report the "Actual
               hash".
            3. Set SHA512 to the "Actual hash"
            4. Run "vcpkg install dimcli" again, it should pass.
    2. Commit ports/dimcli/** files to local repository.
    3. Run "vcpkg x-add-version dimcli"

# Outstanding Issues
None
