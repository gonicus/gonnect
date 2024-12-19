#!/usr/bin/env python3
import re
import os
import sys
import argparse

try:
    import gitlab
except:
    pass

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('source', help='source to analyze')
    parser.add_argument("--token", help="gitlab private access token")
    args = parser.parse_args()

    start = re.compile(r'^([./0-9a-z][^:]+):(\d+):(\d+): warning: (.*) \[-Wclazy-(.*)\]$')
    active = False
    buf = []
    e_file = ""
    e_line = 0
    e_column = 0
    e_note = ""
    e_clazy_ref = ""
    doc = {}
    exit_code = 0

    with open(args.source, "r") as f:
        for _, line in enumerate(f):
            line = line.rstrip()

            # End of warning entry?
            if active and (line.startswith("In ") or "generated." in line):
                active = False
                exit_code = 1
                doc[e_file + "::" + e_line] = ("""##### {title} [{clazy_ref}](https://github.com/KDE/clazy/blob/master/docs/checks/README-{clazy_ref}.md) in [*{short_file}* +{line}]({file}#L{line}):
```c++
{code}
```

""".format(title=e_note.capitalize(), clazy_ref=e_clazy_ref, line=e_line, short_file=e_file, file=e_file, code='\n'.join(buf)))

                print("::warning file={file},line={line},col={col}::{message}".format(
                    file=e_file,
                    line=e_line,
                    col=e_column,
                    message="{title} [{clazy_ref}](https://github.com/KDE/clazy/blob/master/docs/checks/README-{clazy_ref}.md)".format(title=e_note.capitalize(), clazy_ref=e_clazy_ref)
                ))

                buf = []
                continue

            # Start of warning entry?
            m = start.match(line)
            if m:
                if active:
                    exit_code = 1
                    doc[e_file + "::" + e_line] = ("""##### {title} [{clazy_ref}](https://github.com/KDE/clazy/blob/master/docs/checks/README-{clazy_ref}.md) in [*{short_file}* +{line}]({file}#L{line}):
```c++
{code}
```

""".format(title=e_note.capitalize(), clazy_ref=e_clazy_ref, line=e_line, short_file=e_file, file=e_file, code='\n'.join(buf)))
                    print("::warning file={file},line={line},col={col}::{message}".format(
                        file=e_file,
                        line=e_line,
                        col=e_column,
                        message="{title} [{clazy_ref}](https://github.com/KDE/clazy/blob/master/docs/checks/README-{clazy_ref}.md)".format(title=e_note.capitalize(), clazy_ref=e_clazy_ref)
                    ))
                    buf = []

                else:
                    e_file, e_line, e_column, e_note, e_clazy_ref = m.groups()
                    e_file = os.path.normpath(e_file)
                    e_file = re.sub(r'^.*/src/', 'src/', e_file)

                active = not active

            elif active:
                buf.append(line)

    print("Found %d clazy messages" % len(doc))

    server_url = os.environ["CI_SERVER_URL"]
    if server_url:
        gl = gitlab.Gitlab(server_url, private_token=args.token)
        project = gl.projects.get(os.environ["CI_MERGE_REQUEST_PROJECT_ID"])

        mr = project.mergerequests.get(int(os.environ["CI_MERGE_REQUEST_IID"]))
        if mr and len(doc):
            mr.discussions.create({'body': '#### Review of clazy static code analysis\n\n' + ("".join(doc.values()))})

    exit(exit_code)


if __name__ == "__main__":
    main()
