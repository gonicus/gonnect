# Contributing to GOnnect

Thanks for contributing to **GOnnect**!

As a contributor, here are the guidelines we would like you to follow:

- [Code of conduct](#code-of-conduct)
- [How can I contribute?](#how-can-i-contribute)
- [Code Style](#code-style)
- [Sign off](#sign-off)

We also recommend that you read [How to Contribute to Open Source](https://opensource.guide/how-to-contribute).

## Code of conduct

Help us keep GOnnect open and inclusive. Please read and follow our [Code of conduct](CODE_OF_CONDUCT.md).

## How can I contribute

### Translating

GOnnect is accepting translations through Weblate! If you'd like to
contribute with translations, visit the
[Weblate project](https://hosted.weblate.org/engage/gonnect/).

<a href="https://hosted.weblate.org/engage/gonnect/">
<img src="https://hosted.weblate.org/widget/gonnect/gonnect/multi-auto.svg" alt="State of translation" />
</a>

### Coding

The preferred and easiest way to contribute changes to the project is to fork
it on github, and then create a pull request to ask us to pull your changes
into our repo (https://help.github.com/articles/using-pull-requests/)

We use GitHub's pull request workflow to review the contribution, and either
ask you to make any refinements needed or merge it and make them ourselves.

Your PR should have a title that describes what change is being made. This
is used for the text in the Changelog entry by default (see below), so a good
title will tell a user succinctly what change is being made. "fix: bug where
cows had five legs" and, "feat: add support for miniature horses" are examples of good
titles. Don't include an issue number here: that belongs in the description.
Definitely don't use the GitHub default of "Update file.ts".

As for your PR description, it should include these things:

- References to any bugs fixed by the change (in GitHub's `Fixes` notation)
- Describe the why and what is changing in the PR description so it's easy for
  onlookers and reviewers to onboard and context switch. This information is
  also helpful when we come back to look at this in 6 months and ask "why did
  we do it like that?" we have a chance of finding out.
    - Why didn't it work before? Why does it work now? What use cases does it
      unlock?
    - If you find yourself adding information on how the code works or why you
      chose to do it the way you did, make sure this information is instead
      written as comments in the code itself.
    - Sometimes a PR can change considerably as it is developed. In this case,
      the description should be updated to reflect the most recent state of
      the PR. (It can be helpful to retain the old content under a suitable
      heading, for additional context.)
- Include a step-by-step testing strategy so that a reviewer can check out the
  code locally and easily get to the point of testing your change.
- Add comments to the diff for the reviewer that might help them to understand
  why the change is necessary or how they might better understand and review it.

#### Merge Strategy

The preferred method for merging pull requests is squash merging to keep the commit history trim, but it is up to the discretion of the team member merging the change. When merging make sure to leave the default commit title, or at least leave the PR number at the end in brackets like by default.

#### Changelogs

There's no need to manually add Changelog entries: we use information in the
pull request to populate the information that goes into the changelogs our
users see. By default, the PR title will be used for the changelog entry.

As we are using semantic releases, the PR title must follow the
[Angular Commit Message Convetions](https://github.com/angular/angular/blob/main/contributing-docs/commit-message-guidelines.md)
and as we squash merging, this will be the relevant commit message in the `main` branch.

## Code Style

The code style is defined in the [`.clang-format`](.clang-format) file and will be enforced by an action in the CI pipeline.

## Sign off

In order to have a concrete record that your contribution is intentional and you agree to license it under the same terms as the project's license, we've adopted the same lightweight approach that the Linux Kernel (https://www.kernel.org/doc/html/latest/process/submitting-patches.html), Docker (https://github.com/docker/docker/blob/master/CONTRIBUTING.md), and many other projects use: the DCO (Developer Certificate of Origin: http://developercertificate.org/). This is a simple declaration that you wrote the contribution or otherwise have the right to contribute it to GOnnect:

```
Developer Certificate of Origin
Version 1.1

Copyright (C) 2004, 2006 The Linux Foundation and its contributors.
660 York Street, Suite 102,
San Francisco, CA 94110 USA

Everyone is permitted to copy and distribute verbatim copies of this
license document, but changing it is not allowed.

Developer's Certificate of Origin 1.1

By making a contribution to this project, I certify that:

(a) The contribution was created in whole or in part by me and I
    have the right to submit it under the open source license
    indicated in the file; or

(b) The contribution is based upon previous work that, to the best
    of my knowledge, is covered under an appropriate open source
    license and I have the right under that license to submit that
    work with modifications, whether created in whole or in part
    by me, under the same open source license (unless I am
    permitted to submit under a different license), as indicated
    in the file; or

(c) The contribution was provided directly to me by some other
    person who certified (a), (b) or (c) and I have not modified
    it.

(d) I understand and agree that this project and the contribution
    are public and that a record of the contribution (including all
    personal information I submit with it, including my sign-off) is
    maintained indefinitely and may be redistributed consistent with
    this project or the open source license(s) involved.
```

If you agree to this for your contribution, then all that's needed is to include the line in your commit or pull request comment:

```
Signed-off-by: Your Name <your@email.example.org>
```

We accept contributions under a legally identifiable name, such as your name on government documentation or common-law names (names claimed by legitimate usage or repute). Unfortunately, we cannot accept anonymous contributions at this time.

Git allows you to add this signoff automatically when using the `-s` flag to `git commit`, which uses the name and email set in your `user.name` and `user.email` git configs.

If you forgot to sign off your commits before making your pull request and are on Git 2.17+ you can mass signoff using rebase:

```
git rebase --signoff origin/develop
```