#!/usr/bin/env bash
# Audite ce que le cadrage des dialogues a réellement fait pendant une session.
#
#   1. le mécanisme unique (`ODialogFit`, installé dans `VApplication`) écrit dans le journal
#      `INFO … vDialogFit: cannot fit …` quand une dialogue ne peut PAS être ramenée dans la zone
#      utile — widget de taille fixe, layout minimal plus grand que l'écran, grille impossible à
#      envelopper. C'est un défaut, pas un cas accepté ;
#   2. il écrit `INFO … vDialogFit: content made scrollable to fit …` quand il a dû poser une zone
#      défilante. Ce n'est pas un défaut : c'est la liste de ce qui ne tenait pas, et l'ordre de
#      grandeur de ce que l'écran retirait.
#
# Usage :
#   scripts/audit_dialog_fit.sh                 # dernier journal de l'utilisateur courant
#   scripts/audit_dialog_fit.sh --all           # tous les journaux encore présents
#   scripts/audit_dialog_fit.sh /chemin/du.log  # un journal précis
#
# Sortie : 0 si aucune dialogue ne déborde, 1 sinon (c'est le garde-fou de release).

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Le journal vit dans le dossier de configuration de l'application, comme le dit
# `VAbstractApplication::LogDirPath()` : <ConfigLocation>/<organisation>/Logs/<application>.
find_log_dir() {
    local candidates=(
        "$HOME/Library/Preferences/Ostara/Logs/Ostara"   # macOS
        "$HOME/.config/Ostara/Logs/Ostara"               # Linux
        "$HOME/.config/valentina-project.org/Logs/Ostara"
    )
    local candidate
    for candidate in "${candidates[@]}"; do
        if [[ -d "$candidate" ]]; then
            printf '%s\n' "$candidate"
            return 0
        fi
    done
    return 1
}

logs=()
case "${1:-}" in
    --all)
        log_dir="$(find_log_dir)" || { echo "Aucun dossier de journaux trouvé." >&2; exit 2; }
        while IFS= read -r file; do logs+=("$file"); done < <(ls -t "$log_dir"/*.log 2>/dev/null)
        ;;
    "")
        log_dir="$(find_log_dir)" || { echo "Aucun dossier de journaux trouvé." >&2; exit 2; }
        latest="$(ls -t "$log_dir"/*.log 2>/dev/null | head -n 1)"
        [[ -n "$latest" ]] && logs=("$latest")
        ;;
    *)
        [[ -f "$1" ]] || { echo "Journal introuvable : $1" >&2; exit 2; }
        logs=("$1")
        ;;
esac

if [[ ${#logs[@]} -eq 0 ]]; then
    echo "Aucun journal à lire — l'application n'a pas encore tourné." >&2
    exit 2
fi

overflows=0
scrolls=0

for log in "${logs[@]}"; do
    printf '\n=== %s ===\n' "$log"

    while IFS= read -r line; do
        [[ -z "$line" ]] && continue
        overflows=$((overflows + 1))
        printf '  ✗ %s\n' "${line#*vDialogFit: }"
    done < <(grep -F 'vDialogFit: cannot fit' "$log")

    while IFS= read -r line; do
        [[ -z "$line" ]] && continue
        scrolls=$((scrolls + 1))
        printf '  · %s\n' "${line#*vDialogFit: }"
    done < <(grep -F 'vDialogFit: content made scrollable' "$log")

    # L'inventaire écrit à la fermeture : chaque dialogue rencontrée, ce qu'elle demandait, ce que
    # l'écran offrait, et le verdict. C'est la liste complète, fautives comprises.
    if grep -q -F 'vDialogFit: inventory' "$log"; then
        printf '  --- inventaire mesuré ---\n'
        awk '/vDialogFit: inventory/{printing=1} printing&&!/vDialogFit:/{exit} printing{print}' "$log" |
            sed 's/.*vDialogFit: /  /'
    fi
done

printf '\n%d dialogue(s) rendue(s) défilante(s), %d dialogue(s) impossible(s) à cadrer.\n' "$scrolls" "$overflows"

if [[ $overflows -gt 0 ]]; then
    echo "→ Une dialogue impossible à cadrer EST un défaut : la corriger, ne pas l'accepter."
    exit 1
fi

echo "→ Aucune dialogue ne déborde. (Rappel : ce verdict ne vaut que pour les dialogues ouvertes pendant la session.)"
exit 0
