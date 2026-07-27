# Charte du projet

> **Statut : Charte active — progression mise à jour le 27 juillet 2026**

## Mission

Développer et maintenir un fork open source de Skyrim Together Reborn qui renforce la coopération immersive, fournit une campagne commune et construit un framework d’adaptation de mods solo.

## Périmètre

### Inclus

- client et serveur STRE ;
- protocole réseau et états autoritaires ;
- UI coopérative ;
- plugin Alternate Start ;
- bridge CK/Papyrus ↔ STRE ;
- SDK et contrats d’adaptation ;
- contenu narratif et artistique propre au projet ;
- outils de test, logs, documentation et packaging.

### Hors périmètre initial

- monde persistant de type MMO ;
- compatibilité universelle sans intervention du moddeur ;
- réécriture complète des quêtes vanilla ;
- support garanti de toutes les versions de Skyrim et tous les load orders ;
- redistribution d’assets propriétaires sans autorisation.

## Livrables MVP et progression

1. **Trading alpha** — implémenté ; stabilisation et tests d’intégration à poursuivre.
2. **Item Preview interne** — architecture modulaire et second consommateur Character Creation implémentés ; lease manager à faire.
3. **Alternate Start / Character Build** — bootstrap en auberge, RaceMenu, classes, kits, inventaire et sorts canoniques implémentés ; skip Helgen et départ vanilla à faire.
4. **Alternate Start STRE de campagne** — roster, ready check, phases partagées, persistence et reconnexion à faire.
5. **Première intégration first-party** — Character Build démontre un service autoritaire dédié ; adapter générique non stabilisé.
6. **Documentation de contribution** — portail et guides présents ; parcours clean-machine et CI à compléter.

## Gouvernance

### Décisions produit

La direction produit tranche la vision, le périmètre, le ton narratif et les priorités de roadmap après consultation des responsables concernés.

### Décisions d’architecture

Toute décision structurelle est consignée dans un ADR. Le responsable d’architecture est accountable ; les équipes affectées sont consultées.

### Décisions de contenu

La direction narrative valide les personnages et dialogues. La direction artistique valide le style. L’intégration CK valide la faisabilité en jeu.

### Modifications de protocole

Toute modification d’opcode, de schéma sérialisé ou d’état persistant exige : versionnement, tests de round-trip, stratégie de compatibilité et revue serveur/client.

## Règles de fonctionnement

- Une issue doit décrire un résultat observable, pas seulement une activité.
- Une fonctionnalité n’est pas terminée sans tests, logs utiles et documentation.
- Les branches ne doivent pas mélanger refactor massif et changement fonctionnel sans justification.
- Les dépendances cross-team sont écrites dans la Definition of Ready.
- Les données critiques doivent posséder une source de vérité et une stratégie de reprise.

## Critères de santé du projet

- build reproductible ;
- tests automatisés exécutés sur les PR ;
- upstream base identifié sans placeholder ;
- décisions architecturales enregistrées ;
- roadmap liée aux issues ;
- aucun rôle clé dépendant d’une seule personne sans documentation ;
- démos jouables à chaque jalon.
