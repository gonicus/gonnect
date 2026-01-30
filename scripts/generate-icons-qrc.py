#!/usr/bin/env python3

import glob, subprocess

artwork_folder = "resources/artwork"
target_file = "src/icons_generated.qrc"


def main():

    icons_folder = artwork_folder + "/icons/"
    dark_icons_folder = icons_folder + "dark/"

    unthemed_icons = glob.glob(artwork_folder + "/*.svg")
    light_icons = glob.glob(icons_folder + "*.svg")
    dark_icons = glob.glob(dark_icons_folder + "*.svg")


    # Generate dark icons

    for light_icon in light_icons:
        with open(f"{dark_icons_folder}{file_name(light_icon)}", "w", encoding="utf-8") as f:
            subprocess.run(["sed", "s/#232629/#d3dae3/g", light_icon], stdout=f)


    # Generate qrc file

    with open(target_file, "w", encoding="utf-8") as f:
        f.write("<RCC>\n")
        f.write('    <qresource prefix="/icons">\n')

        for path in unthemed_icons + light_icons:
            insert_line(f, path, "        ")

        f.write('    </qresource>\n')
        f.write('    <qresource prefix="/icons/dark">\n')

        for path in dark_icons:
            insert_line(f, path, "        ")

        f.write('    </qresource>\n')
        f.write("</RCC>\n")


def insert_line(file, path, indentation):
    file.write(f'{indentation}<file alias="{file_name(path)}">../{path}</file>\n')


def file_name(file_path):
    return file_path.split("/")[-1]


if __name__ == '__main__':
    main()
