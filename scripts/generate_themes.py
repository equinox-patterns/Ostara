#!/usr/bin/env python3
"""Generate QSS stylesheets from palette.json and a single template.

Usage: python3 generate_themes.py <palette.json> <templates_dir> <output_dir>

palette.json structure (two-level):
  {
    "light": {
      "primitives": { "blanc": "#FFF", "accent": "#8C4560", ... },
      "semantique": { "fond_fenetre": "@gris_50", ... }
    },
    "dark": { ... }
  }

The script resolves @references in semantique using the primitives map,
then substitutes $variables in template.qss.in. The special variable
$theme is replaced with the theme key (e.g. "light", "dark").
"""
import json, os, sys, re


def resolve_colors(primitives, semantique):
    """Resolve semantique aliases: @gris_50 -> #F5F5F6, with cycle detection."""
    ref_pattern = re.compile(r'^@(.+)$')
    resolved = {}
    max_depth = 10

    for name, value in sorted(semantique.items(), key=lambda kv: -len(kv[0])):
        depth = 0
        current = value
        while depth < max_depth:
            m = ref_pattern.match(current)
            if not m:
                break
            target = m.group(1)
            if target in primitives:
                current = primitives[target]
                depth += 1
            elif target in semantique:
                current = semantique[target]
                depth += 1
            else:
                print(f"  ERROR: unresolved reference '@{target}' in '{name}'")
                current = "#FF00FF"  # magenta = error visible
                break
        if current.startswith("@"):
            print(f"  ERROR: circular or unresolvable reference in '{name}': {value}")
            current = "#FF00FF"
        resolved[name] = current

    return resolved


def main():
    palette_path = sys.argv[1]
    templates_dir = sys.argv[2]
    output_dir = sys.argv[3]

    template_path = os.path.join(templates_dir, "template.qss.in")
    if not os.path.exists(template_path):
        print(f"ERROR: template not found: {template_path}")
        sys.exit(1)

    with open(palette_path) as f:
        palette = json.load(f)

    with open(template_path) as f:
        template = f.read()

    os.makedirs(output_dir, exist_ok=True)

    for theme_name, theme_data in palette.items():
        primitives = theme_data.get("primitives", {})
        semantique = theme_data.get("semantique", {})

        # Resolve: @gris_50 -> #F5F5F6
        colors = resolve_colors(primitives, semantique)

        # Add theme name as a variable for resource paths
        colors["theme"] = theme_name

        content = template
        # Substitute $variable_name with resolved color value.
        # Longest keys first so $accent_texte is replaced before $accent.
        for key, value in sorted(colors.items(), key=lambda kv: -len(kv[0])):
            content = content.replace(f"${key}", value)

        output_file = os.path.join(output_dir, f"{theme_name}.qss")
        with open(output_file, "w") as f:
            f.write(content)

        unresolved = [w for w in content.split() if w.startswith("$")]
        if unresolved:
            print(f"  WARNING: unresolved variables in {theme_name}: {unresolved}")

        print(f"  {theme_name}.qss ({len(primitives)} primitives, {len(semantique)} tokens)")


if __name__ == "__main__":
    main()
