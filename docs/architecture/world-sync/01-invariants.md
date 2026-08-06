# World Sync — Invariants

- **Statut : Proposed**
- **Date : 2026-07-30**

## Identité

1. Une entité logique persistante possède exactement un `WorldEntityId`.
2. Un FormID temporaire Skyrim est une liaison locale, jamais une identité réseau durable.
3. Plusieurs clients peuvent représenter le même `WorldEntityId` avec des FormID locaux différents.
4. Une liaison locale devient invalide après chargement de sauvegarde, recréation de référence ou changement de génération explicite.

## Autorité

1. Le serveur STRE décide de l’état logique courant pendant la session.
2. La sauvegarde Skyrim de l’hôte constitue le checkpoint externe canonique du monde vanilla.
3. Le journal STRE protège les mutations validées non encore intégrées au checkpoint hôte.
4. Un invité ne devient jamais source persistante de vérité.
5. Les règles vanilla de suppression et de reset sont observées sur l’hôte puis propagées par le serveur.

## Conservation

1. Un transfert validé conserve les quantités, hors création ou destruction explicite.
2. Une unité d’objet ne peut se trouver simultanément au sol et dans un inventaire.
3. Une unité lootée ne peut être attribuée qu’une fois.
4. Une entité supprimée ne peut pas réapparaître à cause d’un paquet retardé ou d’une ancienne sauvegarde sans opération de restauration explicite.

## Concurrence

1. Toute commande mutante possède un identifiant idempotent.
2. Toute mutation vérifie une révision attendue.
3. Les commandes visant une même frontière de cohérence sont sérialisées.
4. État courant, journal et outbox sont validés dans une seule transaction locale.

## Réconciliation

1. La réconciliation compare un état attendu serveur à des références observées chez l’hôte.
2. Un cas ambigu n’est jamais résolu silencieusement.
3. Une référence restaurée par Skyrim mais supprimée dans l’état canonique est désactivée ou supprimée.
4. Une entité canonique absente du monde hôte est matérialisée lorsque sa cellule devient disponible.
