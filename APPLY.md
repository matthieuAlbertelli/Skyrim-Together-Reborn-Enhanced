# STRE World Sync — jalon 3.8.2

Base: jalon 3.8.1.

## Correctif

Lorsqu'un objet local ne devient pas stable dans les 4 secondes, le client n'abandonne plus son transform.
Il envoie la dernière position disponible avec le log `transform_timeout_fallback`, puis `transform_send ... timeout=true`.

Cela débloque la matérialisation différée sur le client distant. Ce jalon ne prétend pas encore corriger l'impulsion/glissement Havok du drop local.

Extraire à la racine du dépôt, reconstruire et déployer sur les deux clients.
