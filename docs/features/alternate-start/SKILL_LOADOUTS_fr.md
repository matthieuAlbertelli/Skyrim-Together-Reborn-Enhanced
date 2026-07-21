# Alternate Start — Paquetages de compétences

> **Statut : Proposition de conception — V0.1**

## Objectif

Ce document définit le matériel, les sorts, les consommables et les choix d’orientation associés aux compétences privilégiées par une classe.

La classe ne fournit pas directement un paquetage figé. Elle sélectionne des compétences majeures et mineures, puis chaque compétence produit son propre paquetage de départ.

Chaque classe possède deux compétences majeures et quatre compétences mineures.

```text
Classe
→ compétences majeures et mineures
→ paquetage de chaque compétence
→ build final
```

Les valeurs, quantités et objets indiqués ici constituent une base de travail. Ils devront être ajustés après prototypage et tests en jeu.

---

## 1. Contrat commun

Chaque compétence sélectionnée par une classe produit :

```text
socle garanti de matériel et/ou de sorts
+
bonus supplémentaire si la compétence est majeure
```

Une compétence mineure donne :

```text
socle garanti de matériel et/ou de sorts
+
orientation choisie
```

Une compétence majeure donne :

```text
socle garanti
+
bonus majeur
```

Le bonus majeur peut être :

- un objet signature ;
- une amélioration de qualité remplaçant un objet inférieur ;
- des consommables supplémentaires ;
- un second outil complémentaire ;
- un bâton faible pour certaines écoles de magie.

Il ne doit pas nécessairement être identique pour toutes les orientations.

### Nombre d’orientations

Chaque compétence propose au minimum deux orientations.

Certaines compétences en proposent naturellement trois :

```text
Arme à une main
→ épée, hache ou masse

Arme à deux mains
→ espadon, hache de bataille ou marteau

Destruction
→ feu, froid ou foudre

Invocation
→ invocation, arme liée ou nécromancie
```

La structure technique ne doit donc pas coder en dur exactement deux options.

```cpp
struct SkillPackageDefinition
{
    SkillId skillId;
    RewardBundle baseRewards;
    std::vector<SkillOrientationDefinition> orientations;
    BuildVersion version;
};
```

Pour le premier MVP :

```text
minimum : 2 orientations
maximum conseillé : 3 orientations
```

---

# 2. Compétences de guerrier

## 2.1 Arme à une main

### Socle garanti

```text
1 dague de fer
1 potion mineure d’endurance
```

La dague sert d’arme secondaire, même si le joueur choisit une autre famille d’arme.

### Orientation — choisir une arme principale

#### Épéiste

```text
1 épée de fer
```

Style :

```text
rapidité
polyvalence
attaques fréquentes
```

#### Guerrier à la hache

```text
1 hache de guerre en fer
```

Style :

```text
dégâts intermédiaires
pression offensive
style agressif
```

#### Porte-masse

```text
1 masse de fer
```

Style :

```text
coups plus lents
dégâts plus élevés
orientation anti-armure à terme
```

### Bonus si majeure

L’arme principale en fer est remplacée par une version de qualité supérieure contrôlée :

```text
épée, hache ou masse de niveau acier
```

Le remplacement doit être atomique :

```text
retirer l’arme de qualité mineure
→ donner l’arme majeure
```

Il ne faut pas donner les deux.

---

## 2.2 Arme à deux mains

### Socle garanti

```text
2 potions mineures d’endurance
```

### Orientation

#### Bretteur lourd

```text
1 espadon de fer
```

Style :

```text
arme relativement rapide
bonne réactivité
```

#### Ravageur

```text
1 hache de bataille en fer
```

Style :

```text
équilibre entre vitesse et puissance
```

#### Briseur

```text
1 marteau de guerre en fer
```

Style :

```text
attaques lentes
fort impact par coup
```

### Bonus si majeure

L’arme choisie est remplacée par sa version de qualité supérieure.

Le joueur reçoit également :

```text
1 potion mineure d’endurance supplémentaire
```

---

## 2.3 Archerie

### Socle garanti

```text
1 dague de fer
```

Elle représente l’arme de secours du tireur.

### Orientation

#### Tireur mobile

```text
1 arc court de départ
30 flèches de fer
```

Profil recherché :

```text
cadence élevée
dégâts par flèche réduits
mobilité
```

L’arc court pourra être un objet propre au mod afin de maîtriser sa cadence.

#### Chasseur

```text
1 arc de chasse
24 flèches de fer
```

Profil recherché :

```text
cadence plus lente
dégâts par tir supérieurs
économie de munitions
```

### Bonus si majeure

```text
12 flèches de qualité supérieure
1 poison faible applicable à une arme
```

Les flèches supplémentaires doivent rester rares afin que les munitions de qualité conservent de la valeur.

---

## 2.4 Parade

Cette compétence doit fonctionner avec un bouclier, mais aussi avec une arme à deux mains.

### Socle garanti

```text
2 potions mineures d’endurance
```

### Orientation

#### Porte-bouclier

```text
1 bouclier léger
```

Caractéristiques recherchées :

```text
protection modérée
faible poids
coups de bouclier peu coûteux
```

#### Garde renforcée

```text
1 bouclier lourd
```

Caractéristiques recherchées :

```text
meilleure protection
poids supérieur
coups de bouclier plus coûteux
```

#### Garde d’arme

Option offerte aux classes possédant également Arme à deux mains :

```text
1 paire de brassards de garde
1 effet passif faible améliorant le blocage sans bouclier
```

Cet effet ne doit fonctionner que lorsque :

```text
aucun bouclier n’est équipé
et
une arme à deux mains est utilisée
```

### Bonus si majeure

Pour Porte-bouclier ou Garde renforcée, le bouclier est remplacé par une variante mieux fabriquée.

Pour Garde d’arme, les brassards procurent une amélioration légèrement supérieure et une réduction mineure du coût en endurance du blocage.

### Risque technique

La garde d’arme nécessitera probablement :

- un effet magique conditionnel ;
- un perk ;
- ou un script léger vérifiant l’équipement.

Elle devra faire l’objet d’un prototype séparé.

---

## 2.5 Armure lourde

### Socle garanti

```text
1 cuirasse lourde
1 paire de gantelets lourds
1 paire de bottes lourdes
```

Le personnage mineur ne reçoit pas immédiatement un ensemble complet avec casque.

### Orientation

#### Armure allégée

```text
protection légèrement inférieure
poids réduit
meilleure mobilité initiale
```

#### Armure renforcée

```text
protection supérieure
poids plus élevé
gestion de l’endurance plus contraignante
```

Les deux variantes doivent rester classées comme armure lourde.

### Bonus si majeure

```text
1 casque lourd assorti
```

Le personnage obtient ainsi un ensemble complet et reconnaissable.

Le bonus majeur peut également améliorer légèrement la qualité de la cuirasse, sans dépasser le niveau d’un équipement de départ raisonnable.

---

## 2.6 Forgeage

### Socle garanti

```text
1 tablier de forgeron
2 lingots de fer
2 bandes de cuir
```

Le tablier est principalement thématique. Il ne doit pas procurer un bonus important.

### Orientation

#### Fabricant d’armes

```text
2 lingots de fer supplémentaires
2 bandes de cuir supplémentaires
1 pierre à aiguiser ou objet narratif équivalent
```

Le paquetage permet de fabriquer ou d’améliorer une arme simple au premier atelier rencontré.

#### Armurier

```text
2 morceaux de cuir
4 bandes de cuir
1 lingot de fer supplémentaire
```

Le paquetage privilégie la fabrication ou l’amélioration d’armures.

### Bonus si majeure

```text
1 potion faible de forgeage
1 matériau métallique supplémentaire contrôlé
```

Il ne faut pas donner suffisamment de matériaux pour produire immédiatement un équipement complet ou générer un bénéfice marchand important.

---

# 3. Compétences de mage

## 3.1 Destruction

### Socle garanti

```text
Sort : Projectile arcanique mineur
```

Caractéristiques :

```text
faibles dégâts magiques neutres
aucun effet secondaire
faible coût en Magie
```

Il permet d’utiliser immédiatement la compétence, quelle que soit l’orientation élémentaire.

### Orientation

#### Feu

```text
sort projectile
dégâts directs corrects
faibles dégâts persistants
```

Style :

```text
pression offensive
efficacité contre les cibles vivantes
```

#### Froid

```text
dégâts inférieurs
ralentissement
dégâts d’endurance
```

Style :

```text
contrôle des combattants de mêlée
maintien de la distance
```

#### Foudre

```text
dégâts immédiats
dégâts de Magie
coût supérieur
```

Style :

```text
lutte contre les mages
intervention rapide
```

### Bonus si majeure

```text
1 bâton faible correspondant à l’élément choisi
```

Le bâton doit :

- avoir peu de charges ;
- être moins efficace que le sort ;
- servir de réserve lorsque la Magie est épuisée ;
- avoir une faible valeur marchande.

---

## 3.2 Altération

### Socle garanti

```text
Sort : Peau protectrice mineure
```

Effet :

```text
petit bonus d’armure
durée raisonnable
faible coût
```

### Orientation

#### Protection physique

```text
sort de protection physique renforcée
durée plus courte
bonus d’armure supérieur
```

Style :

```text
absorber les attaques martiales
préparer un engagement rapproché
```

#### Protection arcanique

```text
faible résistance magique
bonus d’armure inférieur
```

Style :

```text
affronter les lanceurs de sorts
protection polyvalente
```

### Bonus si majeure

```text
1 focalisateur d’Altération
```

Il peut s’agir d’un objet non revendable donnant :

```text
une très faible réduction du coût des sorts d’Altération
```

Le bonus doit rester de l’ordre de quelques pourcents.

---

## 3.3 Invocation

### Socle garanti

```text
Sort : Invocation d’un familier mineur
2 potions mineures de Magie
```

Le familier doit être utile sans pouvoir résoudre seul les premiers combats.

### Orientation

#### Invocateur

```text
Sort : Invocation d’un esprit animal plus robuste
```

Style :

```text
compagnon temporaire
occupation des ennemis
combat indirect
```

#### Lame liée

```text
Sort : Arme liée novice
```

Style :

```text
combat direct
arme disponible sans poids d’inventaire
dépendance à la Magie
```

L’arme exacte pourra dépendre de la classe :

```text
épée liée
arc lié faible
ou autre variante contrôlée
```

#### Nécromancien

```text
Sort : Réanimation d’un cadavre faible
```

Style :

```text
réutilisation des ennemis vaincus
puissance dépendante de l’environnement
préparation plus lente
```

### Bonus si majeure

Le bonus dépend de l’orientation :

```text
Invocateur
→ bâton d’invocation faible

Lame liée
→ second sort d’arme liée complémentaire

Nécromancien
→ bâton de réanimation faible
```

---

## 3.4 Illusion

### Socle garanti

```text
Sort : Apaisement mineur
```

Il doit avoir :

- une courte portée ;
- une faible limite de niveau ;
- une durée modérée.

### Orientation

#### Inspirateur

```text
Sort : Courage
```

Cible :

```text
compagnon
allié
follower en solo
```

Style :

```text
soutien
renforcement d’un allié
```

#### Provocateur

```text
Sort : Frénésie mineure
```

Style :

```text
faire combattre les ennemis entre eux
désorganiser un groupe
```

#### Infiltrateur

```text
Sort : Pas feutrés
```

Style :

```text
réduction du bruit
infiltration
repositionnement
```

### Bonus si majeure

```text
1 bâton faible d’apaisement
```

Ce bonus reste utilisable quelle que soit l’orientation choisie.

---

## 3.5 Guérison

### Socle garanti

```text
Sort : Soin personnel mineur
Sort : Soin d’autrui mineur
```

Le personnage peut donc toujours :

- se soigner ;
- soigner un allié ou un compagnon.

### Orientation

#### Grâce vive

```text
soin important et immédiat
coût en Magie élevé
incantation rapide
rendement magique faible
```

Style :

```text
intervention d’urgence
sauvetage rapide
gestion stricte de la Magie
```

#### Grâce persistante

```text
soin réparti sur plusieurs secondes
coût inférieur
meilleure efficacité totale
absence de réponse immédiate aux dégâts lourds
```

Style :

```text
anticipation
entretien régulier
économie de Magie
```

### Bonus si majeure

```text
1 bâton de guérison novice
```

Le bâton lance un soin d’autrui faible.

Il doit posséder :

- une puissance modeste ;
- peu de charges ;
- une faible valeur marchande ;
- aucun avantage supérieur aux sorts spécialisés.

---

## 3.6 Enchantement

### Socle garanti

```text
Sort : Capture d’âme mineure
2 gemmes spirituelles insignifiantes vides
```

Cette exception donne un sort techniquement rattaché à l’Invocation, car il est nécessaire pour rendre l’Enchantement immédiatement exploitable.

### Orientation

#### Enchanteur d’armes

```text
1 arme de fer portant un enchantement très faible
```

Le joueur peut :

```text
utiliser l’arme
ou
la désenchanter pour apprendre son effet
```

#### Enchanteur d’équipement

```text
1 bijou ou vêtement portant un enchantement très faible
```

Le joueur peut également l’utiliser ou le désenchanter.

### Bonus si majeure

```text
1 gemme spirituelle inférieure remplie
1 objet non enchanté correspondant à l’orientation choisie
```

Exemples :

```text
arme enchantée + arme vierge
bijou enchanté + bijou vierge
```

Les objets propres au mod doivent avoir une faible valeur afin d’éviter leur revente abusive.

---

# 4. Compétences de voleur

## 4.1 Armure légère

### Socle garanti

```text
1 cuirasse légère
1 paire de bottes légères
1 paire de gantelets légers
```

### Orientation

#### Éclaireur

```text
équipement très léger
protection plus faible
bruit réduit
```

Une variante en fourrure ou peau convient à cette identité.

#### Escarmoucheur

```text
équipement plus protecteur
poids légèrement supérieur
```

Une variante en cuir ou cuir renforcé convient à cette identité.

### Bonus si majeure

```text
1 casque ou capuche légère assortie
```

L’ensemble majeur devient complet.

---

## 4.2 Furtivité

### Socle garanti

```text
1 capuche sombre
1 poison faible
```

La capuche ne doit pas nécessairement être enchantée.

### Orientation

#### Ombrelame

```text
1 dague de fer
1 poison faible supplémentaire
```

Style :

```text
approche rapprochée
attaque furtive
retrait rapide
```

#### Tireur de l’ombre

```text
1 arc léger
12 flèches de fer
```

Style :

```text
attaque furtive à distance
positionnement
gestion des munitions
```

### Bonus si majeure

```text
1 paire de bottes à semelles souples
```

Effet :

```text
réduction très faible du bruit produit
```

L’effet ne doit pas approcher la puissance d’un enchantement de Silence complet.

---

## 4.3 Crochetage

### Socle garanti

```text
12 crochets
```

### Orientation

#### Prévoyant

```text
12 crochets supplémentaires
```

Style :

```text
quantité
droit à l’erreur
pas de bonus passif
```

#### Technicien

```text
4 crochets supplémentaires
1 outil de précision
```

L’outil de précision procure un faible élargissement de la zone correcte lors du crochetage.

Style :

```text
moins de consommables
meilleure précision
```

### Bonus si majeure

Pour Prévoyant :

```text
8 crochets supplémentaires
1 paire de gants donnant un très faible bonus de crochetage
```

Pour Technicien :

```text
outil de précision amélioré
```

### Risque technique

Les outils de précision ne sont pas naturellement représentés comme des objets distincts dans le système vanilla.

Une implémentation satisfaisante demandera probablement :

- un effet magique conditionnel ;
- un perk ;
- ou une intégration plus profonde au mini-jeu de crochetage.

---

## 4.4 Vol à la tire

Cette compétence est difficile à enrichir uniquement avec des objets vanilla.

### Socle garanti

```text
1 tenue civile discrète
10 pièces d’or servant de couverture
```

### Orientation

#### Coupe-bourse

```text
1 paire de gants de doigts agiles
```

Effet :

```text
très faible bonus de vol à la tire
```

#### Détourneur d’attention

```text
3 objets de diversion consommables
```

Utilisation recherchée :

```text
détourner brièvement l’attention d’un PNJ
réduire temporairement sa vigilance
créer une fenêtre pour le vol
```

### Bonus si majeure

Pour Coupe-bourse, les gants utilisent une variante légèrement supérieure.

Pour Détourneur d’attention :

```text
3 objets de diversion supplémentaires
durée légèrement augmentée
```

### Risque technique

Le système de diversion nécessitera probablement :

- un script ;
- une réaction IA contrôlée ;
- des garde-fous dans les villes ;
- des tests pour éviter de casser les packages des PNJ.

Cette compétence ne doit pas être choisie comme première tranche verticale.

---

## 4.5 Éloquence

### Socle garanti

```text
25 pièces d’or
```

### Orientation

#### Diplomate

```text
1 tenue convenable
2 potions faibles d’éloquence
```

Style :

```text
persuasion ponctuelle
présentation sociale
```

#### Marchand

```text
1 tenue de négociant
50 pièces d’or supplémentaires
1 petit lot de marchandises à faible valeur
```

Style :

```text
capital de départ
commerce
gestion économique
```

Le lot de marchandises doit rester peu rentable afin de ne pas créer une source d’or gratuite disproportionnée.

### Bonus si majeure

```text
1 chevalière ou insigne de négociateur
```

Effet :

```text
très faible bonus d’Éloquence
```

L’objet doit avoir une faible valeur et ne pas être cumulable avec une seconde copie.

---

## 4.6 Alchimie

### Socle garanti

```text
1 note de recettes de novice
1 petit ensemble d’ingrédients communs
```

### Orientation

#### Apothicaire

Le joueur reçoit les ingrédients nécessaires pour fabriquer quelques préparations parmi :

```text
restauration de Santé
restauration d’Endurance
régénération faible
```

Il reçoit également :

```text
1 potion mineure de soin prête à l’emploi
```

#### Empoisonneur

Le joueur reçoit les ingrédients nécessaires pour fabriquer quelques préparations parmi :

```text
dégâts de Santé
ralentissement
réduction d’Endurance
```

Il reçoit également :

```text
1 poison faible prêt à l’emploi
```

#### Herboriste de terrain

Le joueur reçoit les ingrédients nécessaires pour fabriquer quelques préparations parmi :

```text
résistance au froid
résistance au feu
restauration d’Endurance
```

Il reçoit également :

```text
1 potion mineure de résistance
```

### Bonus si majeure

```text
1 catalyseur supplémentaire contrôlé
1 second ensemble d’ingrédients correspondant à l’orientation
```

Les ingrédients devront être sélectionnés avec soin afin d’éviter :

- les effets très puissants accessibles accidentellement ;
- les recettes de forte valeur marchande ;
- les combinaisons d’invisibilité trop précoces.

---

# 5. Règles globales d’équipement

## 5.1 Limitation de la puissance initiale

Le paquetage ne doit normalement pas dépasser :

```text
armes de fer ou équivalent
rares améliorations de niveau acier pour les majeures
armures basiques
sorts de niveau novice personnalisés
enchantements très faibles
consommables en petites quantités
```

Le personnage doit sembler préparé, pas accompli.

## 5.2 Budget par compétence

Une compétence mineure devrait donner au maximum :

```text
1 objet ou sort principal
+
1 petit groupe de consommables
```

Une compétence majeure ajoute normalement :

```text
1 objet signature
ou
1 amélioration de qualité
```

Les ensembles d’armure comptent comme un seul paquetage logique.

## 5.3 Gestion des doublons

Les récompenses doivent être fusionnées lorsque cela est logique :

```text
flèches identiques
potions identiques
crochets
matériaux
```

Les sorts ne doivent jamais être ajoutés deux fois.

Les améliorations majeures remplacent les versions mineures :

```text
arme de fer
→ remplacée par arme supérieure
```

et non :

```text
arme de fer
+
arme supérieure
```

## 5.4 Conflits d’emplacements

Priorité d’équipement automatique :

```text
armure lourde ou légère
→ équipement de combat principal

tenues d’Éloquence, Forgeage ou Vol à la tire
→ conservées dans l’inventaire comme tenues alternatives
```

Le système ne doit pas supprimer une tenue secondaire sous prétexte qu’elle n’est pas équipée au départ.

## 5.5 Pas de perks de départ en V1

La première version ne devrait pas attribuer de perks vanilla.

La classe agit par :

```text
bonus initial de compétence
progression accélérée
équipement
sorts
capacités propres au mod strictement nécessaires
```

Cela évite :

- d’altérer trop fortement les arbres vanilla ;
- de créer des prérequis incohérents ;
- de compliquer le changement ou la migration des classes.

---

# 6. Identifiants proposés

Exemples :

```text
competence.arme_une_main.choix.epee
competence.arme_une_main.choix.hache
competence.arme_une_main.choix.masse

competence.archerie.choix.mobile
competence.archerie.choix.chasseur

competence.guerison.choix.soin_immediat
competence.guerison.choix.soin_progressif

competence.invocation.choix.invocateur
competence.invocation.choix.arme_liee
competence.invocation.choix.necromancien
```

Bonus majeurs :

```text
competence.guerison.majeure.baton_novice
competence.furtivite.majeure.bottes_souples
competence.armure_lourde.majeure.ensemble_complet
```

Les identifiants ne doivent jamais contenir :

- de FormID ;
- d’index d’option ;
- de numéro dépendant de l’ordre du menu.

---

# 7. Difficulté technique estimée

## Paquetages simples

Principalement réalisables avec des objets, listes et sorts classiques :

```text
Arme à une main
Arme à deux mains
Archerie
Forgeage
Armure lourde
Armure légère
Alchimie
Enchantement
Éloquence
```

## Paquetages nécessitant des records personnalisés

```text
Destruction
Altération
Invocation
Illusion
Guérison
Furtivité
```

## Paquetages nécessitant probablement une logique spéciale

```text
Parade sans bouclier
Crochetage avec outils de précision
Vol à la tire avec objets de diversion
```

Ces trois derniers ne doivent pas être utilisés pour le premier prototype technique.

---

# 8. Prototype recommandé

La meilleure compétence pour la première tranche verticale est :

```text
Guérison
```

Elle permet de tester en une seule verticale :

```text
attribution de plusieurs sorts garantis
choix entre deux orientations
bonus distinct pour une compétence majeure
attribution d’un bâton
sauvegarde du choix
rechargement sans duplication
suppression propre lors d’un reset de développement
```

Le prototype peut utiliser temporairement une classe de test :

```text
ClassId = test_guerisseur
```

avec :

```text
Guérison majeure
```

Il n’est pas nécessaire de définir immédiatement ses neuf autres compétences pour valider le mécanisme technique.

# 9. Classes

Chaque classe possède :

```text
2 compétences majeures
4 compétences mineures
```

Les classes et compétences d’origine sont celles de Morrowind. Les listes ci-dessous utilisent uniquement les noms français des compétences de Skyrim.

## 9.1 Correspondances communes

| Compétence de Morrowind | Compétence de Skyrim |
|---|---|
| Précision | Archerie |
| Discrétion | Furtivité |
| Sécurité | Crochetage |
| Armurerie | Forgeage |
| Marchandage | Éloquence |
| Lame courte | Arme à une main |
| Lame longue | Arme à une main |
| Hache | Arme à deux mains |
| Arme contondante | Arme à une main ou Arme à deux mains |
| Lance | Arme à deux mains |
| Combat sans armure | Altération ou Guérison |
| Combat à mains nues | Arme à une main ou Parade |
| Armure intermédiaire | Armure légère ou Armure lourde |
| Acrobatie | Armure légère, Furtivité ou Vol à la tire |
| Athlétisme | Armure légère, Parade ou Guérison |
| Mysticisme | Enchantement, Invocation ou Guérison |

Lorsqu’une conversion produit un doublon, seule la compétence absente de Skyrim est remplacée. Une compétence existant déjà dans Skyrim conserve son rang majeur ou mineur.

---

## 9.2 Classes de combat

### Archer
>Les archers sont des combattants spécialisés dans l'affrontement à distance et les déplacements rapides. Leurs manœuvres incessantes et leur habileté à l'arc leur permettent de maintenir l'ennemi loin d'eux jusqu'à ce qu'il soit suffisamment affaibli, après quoi ils l'achèvent au contact.

**Majeures**
- Archerie
- Armure légère

**Mineures**
- Arme à deux mains (une bonne allonge tient les adversaires à distance)
- Furtivité
- Altération (permet de traquer et de voyager sous l'eau)
- Forgeage (l'archer est un guerrier qui entretient ses armes)

### Barbare
>Les barbares sont de fiers guerriers des plaines ou des montagnes. Les civilités ne sont pas leur fort et ils se montrent souvent brutaux et directs. Avides d'actes d'héroïsme, ils excellent dans les combats d'homme à homme.

**Majeures**
- Arme à deux mains
- Parade

**Mineures**
- Furtivité (les barbares privilégient les embuscades)
- Armure légère (privilégie rapidité et endurance, armures de fourrure, peau, cuir)
- Forgeage (les armes sont presque sacrées pour les peuples barbares)
- Archerie (les barbares chassent énormément pour se sustenter)

### Croisé
>Tout guerrier en armure possédant des pouvoirs magiques et agissant au nom d'une noble cause peut se dire croisé. Les croisés ont pour mission de faire le Bien. Traquant monstres et individus malfaisants, ils s'enrichissent tout en débarrassant le monde des forces du Mal.

**Majeures**
- Arme à deux mains
- Destruction

**Mineures**
- Armure lourde
- Parade
- Guérison
- Altération

### Chevalier
>Les chevaliers sont des guerriers civilisés, nés de famille noble ou qui se sont distingués en tournoi ou sur le champ de bataille. Sachant lire et écrire, ils suivent les règles de la courtoisie et le code de la chevalerie. Ils étudient l'art de la guerre, mais aussi ceux de la magie et de la guérison.

**Majeures**
- Arme à une main
- Parade

**Mineures**
- Éloquence
- Guérison
- Enchantement
- Armure lourde

### Roublard
>Les roublards sont des aventuriers et des opportunistes, aussi doués pour s'attirer les ennuis que pour s'en sortir. Faisant autant confiance à leur charme et à leur panache qu'à leur épée ou à leur sens des affaires, ils n'aiment rien tant que les périodes de conflits, s'appuyant sur leur chance et leur ruse pour survivre en tirant profit de toutes les opportunités qui leur sont offertes.

**Majeures**
- Arme à une main
- Éloquence

**Mineures**
- Armure légère
- Vol à la tire (combat tactique: empoisonner à distance, désarmer les ennemis)
- Parade
- Alchimie (poisons)

### Éclaireur
>Leur grande furtivité permet aux éclaireurs de déterminer les meilleurs itinéraires et d'épier l'ennemi. Quand il leur faut se battre, ils le font à l'aide d'arme de jet ou de traits, en employant des tactiques de guérilleros. Contrairement aux barbares, qui se montrent très impulsifs, les éclaireurs combattent de façon prudente et méthodique.

**Majeures**
- Furtivité
- Archerie

**Mineures**
- Arme à une main
- Parade
- Armure légère
- Altération

### Guerrier
>Tous les guerriers sont les combattants professionnels, soldats mercenaires et aventuriers de Tamriel. Formés au maniement de nombreuses armes et au port de divers types d 'armures, ils sont habitués à mettre sans cesse leur vie en jeu et l'effort physique ne leur fait pas peur.

**Majeures**
- Arme à une main
- Arme à deux mains

  **Mineures**
- Parade
- Archerie
- Armure légère
- Armure lourde

---

## 9.3 Classes de furtivité

### Acrobate
> Les acrobates sont en réalités des cambrioleurs de haut vol. Ces voleurs se reposent sur leur discrétion pour éviter de se faire repérer, et sur leur ruse et leur rapidité pour échapper à la garde le cas échéant.

**Majeures**
- Crochetage
- Furtivité

**Mineures**
- Archerie
- Éloquence
- Altération (télékinésie pour voler des objets à distance, combat sans armure)
- Arme à une main

### Agent
>Les agents sont des spécialistes du renseignement, formés à jouer différents rôles pour approcher leur cible. Mais ils savent également se défendre au besoin. Farouchement indépendants et habitués à opérer seuls, ils agissent pour le compte d'un supérieur hiérarchique, au nom d'une cause ou encore pour des raisons qui ne regardent qu'eux.

**Majeures**
- Éloquence
- Furtivité

**Mineures**
- Armure légère
- Arme à une main
- Illusion
- Invocation (l'agent peut avoir besoin de se déplacer sans arme pour éviter d'être repéré, donc il les invoque)

### Assassin
>Les assassins sont des tueurs professionnels faisant confiance à leur discrétion et à leurs grandes facultés de mouvement pour approcher de leur cible sans se faire repérer, après quoi ils l'exécutent à l'aide d'une arme de jet ou, s'il leur faut opérer au contact, d'une arme de petite taille. Malgré la profession qu'ils ont choisis, certains agissent pour la noble cause.

**Majeures**
- Furtivité
- Arme à une main

**Mineures**
- Archerie
- Armure légère
- Vol à la tire (pour empoisonner en silence)
- Alchimie (pour les poisons)

### Barde
>Les bardes sont des conteurs et des gardiens du savoir. Ils recherchent l'aventure pour les connaissances qu'elle leur apporte, faisant confiance à leurs armes et à leurs sorts pour les protéger contre tous les dangers.

**Majeures**
- Éloquence
- Illusion

  **Mineures**
- Alchimie
- Arme à une main
- Parade
- Armure légère

### Moine
>Les moines étudient l'art du combat à mains nues. Dotés d'une grande agilité et d'un sens aigu de la discrétion, ils savent également se battre à l'aide de certaines armes de jet ou de corps à corps.

**Majeures**
- Altération (sorts indispensables pour se battre sans armure)
- Furtivité

  **Mineures**
- Arme à une main
- Guérison
- Parade
- Archerie

### Pèlerin
>Les pèlerins sont des voyageurs en quête d'expériences mystiques. Armes, armues et magie leur permettent de se défendre contre les dangers de la route et leur connaissance approfondie du vaste monde fait d'eux des spécialistes du commerce et de la persuasion.

**Majeures**
- Éloquence
- Archerie

**Mineures**
- Guérison
- Armure légère
- Alchimie
- Arme à deux mains

### Voleur
>Comme son nom l'indique, les voleurs ont pour spécialité de s'emparer des biens des autres mais contrairement aux brigands, ils font usage de ruse et de subtilité plutôt que de force et de violence, à tel point que certains finissent par acquérir une réputation de redresseur de torts auprès de la population.

**Majeures**
- Furtivité
- Vol à la tire

**Mineures**
- Crochetage
- Armure légère
- Arme à une main
- Éloquence

---

## 9.4 Classes de magie

### Mage de guerre
>Les mages de guerre sont des magiciens portés sur les sorts de destruction. Revêtus d'une lourde armure et capables de se défendre l'arme à la main, ils maîtrisent une magie moins diversifiée que les autres lanceurs de sorts et se battent en faisant appel aux éléments et aux créatures convoquées.

**Majeures**
- Destruction
- Armure lourde

**Mineures**
- Invocation
- Altération
- Arme à deux mains
- Enchantement

### Guérisseur
>Les guérisseurs sont des lanceurs de sorts ayant prêté serment de secourir les malades et les blessés. Quand ils se sentent menacés, ils se défendent avec raison, préférant mettre leur adversaire hors de combat et ne le tuant qu'en dernier recours.

**Majeures**
- Guérison
- Altération

**Mineures**
- Armure légère
- Illusion
- Éloquence
- Alchimie

### Mage
>Même si la plupart des mages affirment étudier la magie par pur plaisir intellectuel, ils en tirent souvent un profit plus que substantiel. Leur caractère et leur motivation peuvent varier du tout au tout, mais tous ont un point commun : l'amour de la magie.

**Majeures**
- Destruction
- Altération

**Mineures**
- Invocation
- Illusion
- Guérison
- Enchantement

### Lame noire
>Les lames noires utilisent leurs sorts pour augmenter leurs facultés de discrétion, de déplacement et de combat rapproché. Si leur réputation est sinistre, c'est sans doute parce que la plupart d'entre eux font carrière en tant qu'espions, voleurs ou assassins.

**Majeures**
- Illusion
- Furtivité

**Mineures**
- Destruction
- Arme à une main
- Armure légère
- Altération

### Ensorceleur
>Bien qu'étant des lanceurs de sorts au même titre que les mages, les ensorceleurs se consacrent presque exclusivement aux convocations et aux enchantements. Ils recherchent constamment parchemins, anneaux, armures et armes magiques, et n'aiment rien tant que contrôler morts-vivants et serviteurs daedriques.

**Majeures**
- Enchantement
- Invocation

**Mineures**
- Destruction
- Altération
- Armure lourde
- Arme à une main

### Magelame
>Les magelames sont des lanceurs de sorts apportant leur soutient aux troupes impériales en cas de conflit. Ceux qui quittent l'armée deviennent souvent mercenaires et font d'excellent aventurier.

**Majeures**
- Arme à une main
- Destruction

**Mineures**
- Parade
- Armure lourde
- Guérison
- Altération

### Chasseur de sorcières
>Les chasseurs de sorcières vouent leur existence à l'éradication des cultes malfaisants et de tous les adeptes de la magie profane. Pour ce faire, ils sont formés à toutes les facettes de la lutte contre les vampires, sorcières, sorciers et autres nécromanciens.

**Majeures**
- Invocation
- Archerie

**Mineures**
- Armure légère
- Alchimie
- Enchantement
- Furtivité