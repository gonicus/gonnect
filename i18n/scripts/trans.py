import yaml
from lxml import etree

lang = "it_IT"

en = dict()
other = dict()
short_lang = lang.split("_")[0]

with open("en.yml") as stream:
    try:
        data = yaml.safe_load(stream)
        en = data["en"]["countries"]
    except yaml.YAMLError as exc:
        print(exc)

with open("%s.yml" % short_lang) as stream:
    try:
        data = yaml.safe_load(stream)
        other = data[short_lang]["countries"]
    except yaml.YAMLError as exc:
        print(exc)


root = etree.Element("QPH")
root.set("sourcelanguage", "en_US")
root.set("language", lang)

for key in en:
    phrase = etree.SubElement(root, "phrase")
    source = etree.SubElement(phrase, "source")
    source.text = en[key]
    target = etree.SubElement(phrase, "target")
    target.text = other[key] if key in other else ""

f = open('example.xml', 'wb')
f.write(etree.tostring(root, encoding="UTF-8",
                     xml_declaration=True,
                     pretty_print=True,
                     doctype='<!DOCTYPE QPH>'))
f.close()
