# Versioning

This describes the versioning scheme used for the K4A SDK.

## Versioning Scheme

The K4A sdk will be versioned following [Semantic
Versioning](https://semver.org). All final releases will follow semantic
version [1.0.0](https://semver.org/spec/v1.0.0.html) in order to maintain
compatabilty with nuget support in Visual Studio 2015. Pre-release versions
may have semantic versioning [2.0.0](https://semver.org/spec/v2.0.0.html).

## What Determines the K4A Version String

Sematic versioning is made up of 5 parts.

1) Major

2) Minor

3) Patch

4) Pre-release

5) Build metadata

In the K4A repo, Major, Minor, and Patch are determined by what is in
version.txt. The pre-release and build metadata are determined by the state
of the git repo. The rules are as follows.

If the current commit is tagged with a valid version string, the pre-release
and build metadata values are taken from there. It is also enforced that the
tag's major, minor, and revision numbers match what is in version.txt. If the
repo is dirty for any reason, "dirty" is appended to the build metadata.

If there is no tag, the pre-release and build metadata strings come from the
current branch.

| Branch Name | Repo Dirty | Pre-Release String | Build Metadata String | Notes                                                    |
| ----------- | ---------- | ------------------ | --------------------- | -------------------------------------------------------- |
| master      | No         |                    |                       |                                                          |
| master      | Yes        |                    | dirty                 |                                                          |
| release/*   | No         | beta.{n}           |                       | "n" is the number of commits since branching off develop |
| release/*   | Yes        | beta.{n}           | dirty                 | "n" is the number of commits since branching off develop |
| hotfix/*    | No         | rc.{n}             |                       | "n" is the number of commits since branching off develop |
| hotfix/*    | Yes        | rc.{n}             | dirty                 | "n" is the number of commits since branching off develop |
| develop     | No         | alpha              |.git.{hash}            | "hash" is the git short hash of the current commit       |
| develop     | Yes        | alpha              |.git.{hash}.dirty      | "hash" is the git short hash of the current commit       |
| *           | No         | privatebranch      |                       | For all other branches                                   |
| *           | Yes        | privatebranch      |                       | For all other branches                                   |

If none of the above rules match, the pre-release string is set to
"privatebranch" and the build metadata is empty.

Here are some examples of a versioning string.

* On master branch. Version.txt is 0.1.0. Git repo is clean

  0.1.0

* On master branch. Version.txt is 0.1.0. Git repo is dirty.

  0.1.0+dirty

* On feature branch. Version.txt is 1.2.0. Git repo is dirty.

  1.2.0-privatebranch

* On develop branch. Version.txt is 3.0.1. Git repo is clean.

  3.0.1-alpha+git.bcb533e

* On develop branch. Version.txt is 3.0.1. Git repo is dirty.

  3.0.1-alpha+git.bcb533e.dirty

* On branch release/0.5.0. Version.txt is 0.5.0. Git repo is clean.

  0.5.0-beta.1

* On branch release/0.5.0. Version.txt is 0.5.0. Git repo is dirty.

  0.5.0-beta.1+dirty,

* On branch hotfix/3.1.2. Version.txt is 3.1.2. Git repo is clean.

  3.1.2-rc.4