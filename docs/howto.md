<!--
Copyright Glen Knowles 2022.
Distributed under the Boost Software License, Version 1.0.
-->

# Release checklist
1. Make sure tests pass
    - bin\cli --test
2. Verify code samples in the guide.
    - docgen test
3. Change "Unreleased" section in CHANGELOG.md to the new version.
4. Add new version to docs\docgen.xml.
5. Push to github.
6. Wait for CI to succeed.
7. Draft a new release on github.
    1. Goto Releases, click on "Draft a new release"
    2. Select "Choose a tag" and create the new tag "v{major}.{minor}.{patch}".
    3. Write brief description and publish the release.
8. Publish the docs
    1. Run "docgen site"
    2. Goto vendor\gh-pages directory
    3. Review and push the new docs with description of "Update docs"
