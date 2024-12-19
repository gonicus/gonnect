#!/usr/bin/env python3
import os
import plistlib
import argparse
import re

try:
    import gitlab
except:
    pass

def get_plist_with_content(dir):
    plist_list = []

    for plist in os.listdir(dir):
        if not '.' in plist:
            continue

        plist_name, _ = plist.split('.')
        if plist.endswith('.plist'):
            try:
                with open(dir+'/'+plist, 'rb') as fp:
                    pl = plistlib.load(fp)
            except FileNotFoundError as err:
                print(err)
                exit()
            if pl['diagnostics']:
                plist_list.append(pl)
    if not plist_list:
        print('in %s are no .plist files or there is no \
              content in the .plist files' % dir)

    return plist_list


def filter_exclude(args, file):
    bool_res = False

    if args:
        for arg in args:
            rx = re.compile(arg)
            if rx.match(file):
                bool_res = True
                break
            else:
                bool_res = False
    else:
        bool_res = False

    return bool_res    


if __name__ == '__main__':
    exit_code = 0
    parser = argparse.ArgumentParser()
    parser.add_argument('dir', help='search directory for plist-files')
    parser.add_argument("--token", help="gitlab private access token")
    parser.add_argument('--exclude', action='append', help='regex to exclude path')
    args = parser.parse_args()

    plist = get_plist_with_content(args.dir)

    groups = {}


    for pl in plist:
        if not filter_exclude(args.exclude, pl['files'][pl['diagnostics'][0]['location']['file']]):
            group_item = {
                'file': pl['files'][pl['diagnostics'][0]['location']['file']],
                'type': pl['diagnostics'][0]['type'],
                'issue': pl['diagnostics'][0]['issue_context'] if 'issue_context' in pl['diagnostics'][0] else '',
                'position': '['+str(pl['diagnostics'][0]['location']['line'])+':'+str(pl['diagnostics'][0]['location']['col'])+']',
                'extended_message': pl['diagnostics'][0]['path'][len(pl['diagnostics'][0]['path'])-1]['extended_message']
            }

            if pl['diagnostics'][0]['category'] in groups.keys():
                groups[pl['diagnostics'][0]['category']].append(group_item)
            else:
                groups[pl['diagnostics'][0]['category']] = [group_item]

            exit_code = 1

    doc = []
    count = 0
    for group in groups:
        doc.append('### %s\n' % group)
        for item in groups[group]:
            count += 1
            doc.append('In file `%s %s`, function *%s*:\n```\n%s\n```\n'
                  % (item['file'],
                     item['position'],
                     item['issue'],
                     item['extended_message']))

    print("Found %d clang messages" % count)
    if count > 0:

        server_url = os.environ["CI_SERVER_URL"]
        if server_url:
            gl = gitlab.Gitlab('https://gitlab.intranet.gonicus.de', private_token=args.token)
            project = gl.projects.get(os.environ["CI_MERGE_REQUEST_PROJECT_ID"])

            mr = project.mergerequests.get(int(os.environ["CI_MERGE_REQUEST_IID"]))
            if mr and len(doc):
                mr.discussions.create({'body': '#### Review of clang static code analysis\n\n' + ("".join(doc))})
        elif len(doc):
            with open(os.environ['GITHUB_STEP_SUMMARY'], 'a') as fh:
                print('#### Review of clang static code analysis\n\n' + ("".join(doc)), file=fh)

    exit(exit_code)

