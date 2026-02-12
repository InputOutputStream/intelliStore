-- =========================================================
-- Initialisation de la base Smart Store
-- =========================================================

-- Sécurité : encodage propre
SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- 1. Création de la base
CREATE DATABASE IF NOT EXISTS smart_store
  CHARACTER SET utf8mb4
  COLLATE utf8mb4_general_ci;

USE smart_store;

-- 2. Suppression des tables dans le bon ordre (FK)
DROP TABLE IF EXISTS paniers_actifs;
DROP TABLE IF EXISTS factures;
DROP TABLE IF EXISTS produits;
DROP TABLE IF EXISTS clients;

-- 3. Table clients
CREATE TABLE clients (
    id_client INT AUTO_INCREMENT PRIMARY KEY,
    nom VARCHAR(100) NOT NULL
) ENGINE=InnoDB;

-- 4. Table produits
CREATE TABLE produits (
    id_produit INT AUTO_INCREMENT PRIMARY KEY,
    libelle VARCHAR(100) NOT NULL,
    prix DECIMAL(10,2) NOT NULL CHECK (prix >= 0)
) ENGINE=InnoDB;

-- 5. Table factures
CREATE TABLE factures (
    id_facture INT AUTO_INCREMENT PRIMARY KEY,
    id_client INT NOT NULL,
    montant_total DECIMAL(10,2) NOT NULL CHECK (montant_total >= 0),
    statut_paiement VARCHAR(20) NOT NULL,
    date_facture DATETIME NOT NULL,

    CONSTRAINT fk_factures_client
      FOREIGN KEY (id_client)
      REFERENCES clients(id_client)
      ON DELETE CASCADE
      ON UPDATE CASCADE
) ENGINE=InnoDB;

-- 6. Table paniers_actifs
CREATE TABLE paniers_actifs (
    id INT AUTO_INCREMENT PRIMARY KEY,
    id_client INT NOT NULL,
    id_produit INT NOT NULL,

    CONSTRAINT fk_panier_client
      FOREIGN KEY (id_client)
      REFERENCES clients(id_client)
      ON DELETE CASCADE
      ON UPDATE CASCADE,

    CONSTRAINT fk_panier_produit
      FOREIGN KEY (id_produit)
      REFERENCES produits(id_produit)
      ON DELETE CASCADE
      ON UPDATE CASCADE
) ENGINE=InnoDB;

-- Réactiver les contraintes
SET FOREIGN_KEY_CHECKS = 1;

-- =========================================================
-- INSERTION DES DONNÉES
-- =========================================================

-- Client
INSERT INTO clients (id_client, nom) VALUES (1, 'Alice');

-- Produits
INSERT INTO produits (id_produit, libelle, prix) VALUES 
(1, 'Support telephone', 1.50),
(101, 'Bouteille', 2.50),
(102, 'telephone', 25.00);

-- Factures
INSERT INTO factures (id_facture, id_client, montant_total, statut_paiement, date_facture) VALUES
(1, 1, 78.00, 'paye', '2026-02-08 11:22:57'),
(2, 1, 28.00, 'paye', '2026-02-08 11:28:57'),
(3, 1, 26.50, 'paye', '2026-02-08 11:30:16'),
(4, 1, 25.00, 'paye', '2026-02-08 11:44:38');

-- Panier actif (on force les IDs comme tu voulais)
INSERT INTO paniers_actifs (id, id_client, id_produit) VALUES
(40, 1, 1),
(41, 1, 1),
(42, 1, 102),
(43, 1, 102),
(44, 1, 1),
(45, 1, 102);

-- =========================================================
-- Vérification
-- =========================================================

SELECT '--- TABLES CRÉÉES AVEC SUCCÈS ---' AS Status;
SHOW TABLES;

SELECT * FROM clients;
SELECT * FROM produits;
SELECT * FROM factures;
SELECT * FROM paniers_actifs;
