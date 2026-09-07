#!/bin/bash
# ============================================================================
#  lupdate_ostara.sh — extraction canonique des chaînes (.ts) pour Ostara
#
#  Usage :
#     ./scripts/lupdate_ostara.sh                      (depuis n'importe où)
#     LUPDATE=/chemin/vers/lupdate ./scripts/lupdate_ostara.sh
#     ./scripts/lupdate_ostara.sh /chemin/vers/lupdate
#
#  Pourquoi ce script :
#   - lupdate avec une liste PARTIELLE de sources fait « disparaître » (vanish)
#     les entrées des fichiers non listés (incident du 17-08-2026). La liste est
#     donc TOUJOURS complète : tous les fichiers sources suivis par git
#     (git ls-files) — reproductible et identique à chaque run.
#   - Lancé depuis la racine du dépôt → chemins <location> reproductibles.
#
#  NB : committez (ou au moins git add) vos nouveaux fichiers sources AVANT de
#  lancer ce script : tant qu'un fichier n'est pas suivi par git, ses chaînes
#  ne sont pas extraites.
#
#  IMPORTANT — bruit <location> :
#   Un lupdate complet réécrit <location line="N"> pour tout message dont le
#   numéro de ligne a bougé dans le code. C'est du BRUIT, pas une perte.
#   Pour juger le diff réel : ./scripts/lupdate_diff.sh
# ============================================================================

set -euo pipefail

# Se placer à la racine du dépôt Ostara (ce script vit dans Ostara/scripts/).
cd "$(dirname "$0")/.."

# Résoudre le binaire lupdate : argument > $LUPDATE > défaut Qt 6.11 d'Ostara.
LUPDATE="${1:-${LUPDATE:-$HOME/Qt/6.11.1/macos/bin/lupdate}}"
if [ ! -x "$LUPDATE" ]; then
    echo "ERREUR : lupdate introuvable : $LUPDATE" >&2
    echo "         Passez le chemin en 1er argument ou exportez LUPDATE=…" >&2
    exit 1
fi

# Liste complète et déterministe des sources (fichiers suivis par git).
sources="$(mktemp)"
trap 'rm -f "$sources"' EXIT
git ls-files '*.cpp' '*.h' '*.hpp' '*.cxx' '*.ui' '*.qml' '*.js' > "$sources"
echo "Sources scannées : $(wc -l < "$sources" | tr -d ' ')"

echo "lupdate : $LUPDATE"
"$LUPDATE" "@$sources" -ts share/translations/ostara_*.ts

echo "Terminé. Contrôlez le diff RÉEL avec : ./scripts/lupdate_diff.sh"
