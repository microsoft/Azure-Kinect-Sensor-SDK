# Contributing to the Azure Kinect SDK

The Azure Kinect SDK team welcomes community feedback and contributions. This repo
is relatively new and team members are actively defining and refining the process. Feel free to point out any 
discrepancies between documented process and the actual process.

## Reporting issues and suggesting new features

If the Azure Kinect SDK is not working the way you expect it to, then please report that with a GitHub Issue.

### Filing a bug

Please review the list of open Issues to see if one is already open. Please review all categories, Bugs and 
Enhancements. Also check for closed Issues before opening a new one.

When opening a new issue be sure to document:

* Steps to reproduce the error
* Expected results
* Actual results
* SDK version
* Firmware version
* Log Results from the repro (if possible)

### Requesting new features

Please review the list of open Issues to see if one is already open. Please review all categories, Bugs and 
Enhancements. Also check for Closed Issues before opening a new one.

## Finding issues you can help with

Looking for something to work on? Issues marked [``Good First Issue``](https://github.com/Microsoft/Azure-Kinect-Sensor-SDK/labels/good%20first%20issue) 
are a good place to start.

You can also check the [``Help Wanted``](https://github.com/Microsoft/Azure-Kinect-Sensor-SDK/labels/help%20wanted) tag to 
find other issues to help with. If you're interested in working on a fix, leave a comment to let everyone know and to help 
avoid duplicated effort from others.

Once you are committed to fixing an issue, assign it to yourself so others know the issue has an owner.

## Contributing code changes

We welcome your contributions, especially to fix bugs and to make improvements which address the top Issues. Some general 
guidelines:

* **DO** create one pull request per Issue, and ensure that the Issue is linked in the pull request.
* **DO** follow our [Coding and Style](#Style-Guidelines) guidelines, and keep code changes as small as possible.
* **DO** include corresponding tests whenever possible.
* **DO** check for additional occurrences of the same problem in other parts of the codebase before submitting your PR.
* **DO** [link the Issue](https://github.com/blog/957-introducing-issue-mentions) you are addressing in the pull request.
* **DO** write a good description for your pull request. More detail is better. Describe why the change is being made and 
why you have chosen a particular solution. Describe any manual testing you performed to validate your change.
* **DO NOT** submit a PR unless it is linked to an Issue marked Triage Approved. This enables us to have a discussion on 
the idea before anyone invests time in an implementation.
* **DO NOT** merge multiple changes into one PR unless they have the same root cause.
* **DO NOT** submit pure formatting/typo changes to code that has not been modified otherwise.

**NOTE:** *Submitting a pull request for an approved Issue is not a guarantee it will be approved. The change must meet 
our high bar for code quality, architecture, and performance.*

# Making changes to the code

## Building
Check out how to set up your environment and do a build [here](docs/building.md).

## Style Guidelines
The public API surface is written in C. Internally most of the code is C, C++ is used only when necessary, such as when 
leveraging other open source projects that are written for C++.

Style formatting is enforced as part of check in criteria using [.clang-format](.clang-format).

See [standards](docs/standards.md) for more information.

## Testing
To complete a PR all tests must pass. Please see [testing.md](docs/testing.md) for more information.

## Workflow for Submitting a Change

Azure Kinect uses the [GitHub flow](https://guides.github.com/introduction/flow/) where most
development happens on the `develop` branch. The `develop` branch should always be in a
healthy state.

1) Start with an issue that has been tagged with Triage Approved. Otherwise, create an Issue and
start a conversation with the Azure Kinect Team to get the Issue Triage Approved.
1) If you have not already, fork the repo.
1) Make changes.
1) Test the change, all tests should pass:
   * ` CTest -L unit `
   * ` CTest -L function`
   * ` CTest -L perf`
1) Create a pull request.
   * The PR description must reference the issue.
1) An Azure Kinect SDK team member will review the change. See the [review process](#review-process) for more information.
   * 1 team member must sign off on the change.
   * Other reviewers are welcome.
1) After the change has been reviewed by a team member, the PR will be submitted to the Azure Kinect CI system for official build and testing.
   * In the event of a test failure, the PR owner must address and provide a new PR commit and the review process starts over.
1) Once the change is signed off by a team member and the test pass shows no regressions, an Azure Kinect team member will complete the PR.

**NOTE:** *Any update to the pull request after approval requires additional review and test pass.*

When completing a pull request, we will generally squash your changes into a single commit. Please
let us know if your pull request needs to be merged as separate commits.

## Review Process
After submitting a pull request, members of the Azure Kinect team will review your code. We will
assign the request to an appropriate reviewer. Any member of the community may
participate in the review, but at least one member of the team will ultimately approve
the request.

Often, multiple iterations will be needed to respond to feedback from reviewers. Try looking at
[past pull requests](https://github.com/Microsoft/Azure-Kinect-Sensor-SDK/pulls?q=is%3Apr+is%3Aclosed) to see what the 
experience might be like.

After at least one member of the Azure Kinect team has approved your request, it will be merged by a member of the Azure
Kinect team. For Pull Requests where feedback is desired but the change is not complete, please mark the Pull Request
as a draft.

# Contributor License Agreement
Most contributions require you to agree to a Contributor License Agreement (CLA) declaring that you have
the right to, and actually do, grant us the rights to use your contribution. For details, visit https://cla.microsoft.com.

When you submit a pull request, a CLA-bot will automatically determine whether
you need to provide a CLA and decorate the PR appropriately (e.g., label,
comment). Simply follow the instructions provided by the bot. You will only
need to do this once across all repositories using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). 
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact 
[opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.
