# Processus de release

> **Statut : Proposition**

## Branches

- `dev` : intégration ;
- `main` : releases préparées ;
- tags SemVer ;
- branches feature courtes.

## Préparation

1. geler les schemas/protocoles ;
2. identifier le commit upstream ;
3. exécuter tests C++, UI et scénarios manuels ;
4. produire changelog ;
5. vérifier licences ;
6. construire client, serveur, UI et plugin CK ;
7. générer checksums ;
8. smoke test sur installation propre.

## Package

Séparer :

- mod joueur ;
- serveur ;
- symboles de debug ;
- SDK/documentation ;
- sources sous licence.

## Versionnement

- patch : correction compatible ;
- minor : capability ou feature compatible ;
- major : rupture de protocole, snapshot ou adapter API.

Les adapters ont leur propre version de schéma négociée avec STRE.

## Rollback

Toute release doit documenter : sauvegardes compatibles, downgrade autorisé ou non, migrations irréversibles et procédure de restauration.
