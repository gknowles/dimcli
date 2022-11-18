<!--
Copyright Glen Knowles 2022.
Distributed under the Boost Software License, Version 1.0.
-->

# Release checklist
1. Be on the dev branch.
2. Make sure tests pass.
    - bin\cli --test
3. Verify code samples in the guide.
    - docgen test
4. Change "Unreleased" section in CHANGELOG.md to the new version.
5. Add new version to docs\docgen.xml.
6. Push to github.
7. Wait for CI to succeed.
8. Merge to master.
    1. git fetch origin master # fetch any possible changes from master
    2. git merge master # merge any changes from master back to dev
    2. git push origin dev:master # update master head to same as dev
9. Draft a new release on github.
    1. Goto Releases, click on "Draft a new release"
    2. Select "Choose a tag" and create the new tag "v{major}.{minor}.{patch}".
    3. Set title to the same as the new tag.
    4. Write brief description and publish the release.
10. Publish the docs
    1. Run "docgen site".
    2. Goto vendor\gh-pages directory.
    3. Review, commit, and push the new docs with description of "Update docs".
