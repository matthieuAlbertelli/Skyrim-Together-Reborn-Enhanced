# Matrice de compatibilité

> **Statut : source de vérité de compatibilité connue**
> **Dernière mise à jour : 11 août 2026**

## Plateforme de référence

| Composant | Version/support observé | État |
|---|---|---|
| Skyrim SE/AE runtime | `1.6.1170` Steam | runtime principal de développement et cible de l'alpha publique |
| Skyrim Together Reborn upstream | baseline historique dans `UPSTREAM.md` | fork intégré dans STRE |
| STRE | `0.2.0-alpha.1` | première alpha publique, build CI Windows validé |
| Angular | 16.x | utilisé |
| xmake | `3.0.9` en CI (`>= 3.0.0` projet) | build Windows validé |
| Creation Kit | environnement compatible `1.6.1170` | utilisé |
| SKSE64 | `2.2.6` pour Skyrim `1.6.1170` | cible d'installation alpha |
| Better Grabbing | `1.17` | plugin externe requis par défaut pour manipulation World Sync multijoueur |
| Address Library | `11` | dépendance externe requise; base runtime pour STRE/Better Grabbing |

Les versions ci-dessus sont les **cibles de release `v0.2.0-alpha.1`**. Une future release doit réviser cette table lorsqu'une dépendance ou un runtime supporté change.

## Fonctionnalités

| Configuration | Trading | World Sync drops | World Sync placed/grab | Character Build solo | Character Build STRE |
|---|---:|---:|---:|---:|---:|
| 1 joueur hors ligne | N/A | vanilla/local | Better Grabbing local | Oui | N/A |
| 2 joueurs | Alpha | Validé | Validé sur cas courants | N/A | Smoke-testé |
| 4 joueurs | À tester | À tester | À tester | N/A | À tester |
| SkyUI | À retester | N/A | N/A | environnement dev | environnement dev |
| Anniversary content | À tester | runtime 1.6.1170 | runtime 1.6.1170 | runtime 1.6.1170 | runtime 1.6.1170 |

## World Sync validé

- drop dynamique → matérialisation distante;
- Havok local + settlement autoritaire;
- pickup distant;
- grab/release d’un WorldEntity droppé;
- lazy adoption d’une référence placée mobile;
- grab/release placé sans crash observateur;
- ownership déclenchant le vol vanilla au grab;
- forced release à l’ouverture du dialogue de garde;
- nage après correction de régression STRE.

## World Sync à étendre

- objets de quête;
- références fortement scriptées;
- enable-parent complexes;
- reset de cellule;
- noms personnalisés d’items;
- persistence durable après restart/save branch;
- 4 joueurs.

## Fiche de test d’un mod/dépendance

Pour chaque mod/dépendance :

- nom/version;
- runtime Skyrim;
- load order si pertinent;
- résultat solo;
- résultat STRE;
- conflit/workaround;
- logs;
- date;
- SHA STRE.
