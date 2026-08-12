# World Sync

> **Statut : Implémenté et validé en jeu pour le périmètre décrit**
> **Validation principale : 10 août 2026**

World Sync fournit une identité et un lifecycle réseau aux objets physiques qui doivent exister de manière cohérente chez plusieurs joueurs, tout en laissant Skyrim simuler localement la physique.

## Comportement utilisateur actuel

### Objet lâché par un joueur

```text
drop local
→ création WorldEntity
→ matérialisation distante
→ Havok local sur chaque client
→ settlement par l'autorité
→ correction ponctuelle uniquement si divergence significative
```

### Objet déjà présent dans le monde

```text
première interaction réseau pertinente
→ lazy adoption via PlacedReferenceId
→ WorldEntityId serveur unique
→ binding sur la TESObjectREFR locale existante
```

Aucun scan global des cellules n’est requis.

### Grab avec Better Grabbing

```text
grab local
→ autorité serveur
→ objet masqué chez les observateurs
→ déplacement local uniquement
→ release
→ réapparition/repositionnement distant
→ Havok local
→ settlement final
```

STRE ne redistribue pas Better Grabbing. Le plugin SKSE est une dépendance multijoueur externe contrôlée via la politique générique `NativePlugins`.

### Ownership / vol

L’ownership est transporté comme provenance (`ExtraOwnerId`) dans les chemins supportés. Grabber une référence possédée sans être autorisé déclenche le comportement de vol Skyrim.

Si un dialogue s’ouvre pendant un grab, STRE force la fin propre du grab afin que les contrôles/dialogues restent utilisables.

## Sources de vérité

- [Technical design](TECHNICAL_DESIGN.md)
- [Protocol reference](PROTOCOL_REFERENCE.md)
- [Better Grabbing integration](BETTER_GRABBING_INTEGRATION.md)
- [Test plan](TEST_PLAN.md)
- [ADR-0017](../../architecture/ADRs/ADR-0017-world-entity-authority-local-havok.md)

## Principes

- `WorldEntityId` est l’identité réseau de l’instance monde.
- Un FormID temporaire local n’est jamais l’identité réseau.
- Une référence placée conserve sa référence Skyrim locale; elle n’est pas dupliquée.
- Le serveur arbitre lifecycle/authority.
- Havok reste local.
- Le réseau force une convergence finale, pas une simulation physique frame-by-frame.
- Les mutations Skyrim reçues du réseau passent par un contexte moteur sûr.
- Une opération incapable de préserver une métadonnée requise doit échouer explicitement.

## Limites connues

- noms personnalisés (`ExtraTextDisplayData`) non synchronisés;
- persistance durable après redémarrage/save branch non implémentée;
- couverture in-game encore à étendre pour objets de quête et références fortement scriptées;
- le modèle n’est pas encore généralisé à tous les types d’entités monde.
