# libk4a Release Process

## Release Steps

When creating a release milestone, you should send meeting invites to maintainers to book the release day and the previous day.
This is to make sure they have enough time to work on the release.

The following release procedure should be started on the previous day of the target release day.
This is to make sure we have enough buffer time to publish the release on the target day.

Before starting the following release procedure, open an issue and list all those steps as to-do tasks.
Check the task when you finish one.
This is to help track the release preparation work.

> Note: Step 2, 3 and 4 can be done in parallel.

1. Create a branch named `release-<Release Tag>` in our private repository.
   All release related changes should happen in this branch.
1. Prepare packages
   - [Build release packages](#building-packages).
   - Sign the MSI packages and DEB/RPM packages.
   - Install and verify the packages.
1. Update documentation, scripts and Dockerfiles
   - Summarize the change log for the release. It should be reviewed by PM(s) to make it more user-friendly.
   - Update [CHANGELOG.md](../../CHANGELOG.md) with the finalized change log draft.
   - Update other documents and scripts to use the new package names and links.
1. Verify the release Dockerfiles.
1. [Create NuGet packages](#nuget-packages) and publish them to [libk4a-core feed][ps-core-feed].
1. [Create the release tag](#release-tag) and push the tag to `libk4a/libk4a` repository.
1. Create the draft and publish the release in Github.
1. Merge the `release-<Release Tag>` branch to `master` in `libk4a/libk4a` and delete the `release-<Release Tag>` branch.
1. Publish Linux packages to Microsoft YUM/APT repositories.
1. Verify the generated docker container images.


## Building Packages

TODO: How do we build release packages?

## Release Tag

LibK4A releases use [Semantic Versioning][semver].
For example until we hit 6.0, each sprint results in a bump to the build number,
so `v6.0.0-alpha.7` goes to `v6.0.0-alpha.8`.

When a particular commit is chosen as a release,
we create an [annotated tag][tag] that names the release.
An annotated tag has a message (like a commit),
and is *not* the same as a lightweight tag.
Create one with `git tag -a v6.0.0-alpha.7 -m <message-here>`,
and use the release change logs as the message.
Our convention is to prepend the `v` to the semantic version.
The summary (first line) of the annotated tag message should be the full release title,
e.g. 'v6.0.0-alpha.7 release of libk4a'.

When the annotated tag is finalized, push it with `git push --tags`.
GitHub will see the tag and present it as an option when creating a new [release][].
Start the release, use the annotated tag's summary as the title,
and save the release as a draft while you upload the binary packages.

[semver]: http://semver.org/
[tag]: https://git-scm.com/book/en/v2/Git-Basics-Tagging
[release]: https://help.github.com/articles/creating-releases/
