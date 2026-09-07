#!/bin/bash
# ============================================================================
#  lupdate_diff.sh — diff « réel » des .ts après un lupdate
#
#  Un lupdate complet renumérote <location line="N"> de tous les messages dont
#  la ligne a bougé dans le code. Ce bruit fait paraître le diff énorme alors
#  qu'aucune traduction n'est perdue.
#  Ce script masque les lignes <location> et n'affiche que les VRAIS changements
#  (ajouts/suppressions de <source> / <translation> / <message>).
#
#  Usage : ./scripts/lupdate_diff.sh            (depuis la racine ou n'importe où)
#  Sortie : les vraies lignes changées, ou « aucun vrai changement » quand le
#           diff n'est que du bruit de renumérotation <location>.
# ============================================================================

set -euo pipefail
cd "$(dirname "$0")/.."

if [ -z "$(git diff --name-only -- share/translations)" ]; then
    echo "Aucun changement dans share/translations."
    exit 0
fi

# grep -E '^[+-]'                 -> lignes ajoutées/supprimées uniquement
# grep -v '^[+-][+-]'             -> retire les en-têtes --- / +++
# grep -v '<location[^>]*>'       -> retire le bruit de renumérotation
git diff -- share/translations \
    | grep -E '^[+-]' \
    | grep -v -E '^[+-][+-]' \
    | grep -v -E '^[+-][[:space:]]*<location[^>]*>' \
    || true
