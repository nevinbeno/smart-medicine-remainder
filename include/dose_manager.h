#ifndef DOSE_MANAGER_H
#define DOSE_MANAGER_H

#include <stdint.h>
#include <stddef.h>
#include <string>
#include <vector>

enum class DoseStatus : uint8_t {
    Scheduled,
    Due,
    Confirmed,
    Missed
};

enum class DoseResult : uint8_t {
    Confirmed,
    ConfirmedLowStock,
    ConfirmedStockEmpty,
    NoDueDose,
    MissedDoseNotCounted,
    UnknownCell
};

struct CellInventory {
    uint8_t cellId;
    std::string medicineName;
    uint16_t stock;
    uint16_t lowStockThreshold;

    bool isLow() const {
        return stock <= lowStockThreshold;
    }
};

struct DoseOccurrence {
    uint16_t doseId;
    uint8_t cellId;
    uint64_t scheduledAt;
    uint16_t units;
    DoseStatus status;
    bool missedAlertPending;
};

// Tracks scheduled dose occurrences and the stock held by each medicine cell.
// The caller supplies Unix timestamps from the RTC and persists data separately
// if state must survive a power loss.
class DoseManager {
public:
    explicit DoseManager(uint32_t missedGraceSeconds = 15UL * 60UL)
        : missedGraceSeconds_(missedGraceSeconds), missedAlertPending_(false) {}

    bool addCell(uint8_t cellId, const std::string& medicineName,
                 uint16_t initialStock, uint16_t lowStockThreshold) {
        if (findCell(cellId) != NULL) {
            return false;
        }

        CellInventory cell = {cellId, medicineName, initialStock, lowStockThreshold};
        cells_.push_back(cell);
        return true;
    }

    bool addDose(uint16_t doseId, uint8_t cellId, uint64_t scheduledAt,
                 uint16_t units = 1) {
        if (findCell(cellId) == NULL || units == 0 || findDose(doseId) != NULL) {
            return false;
        }

        DoseOccurrence dose = {
            doseId, cellId, scheduledAt, units, DoseStatus::Scheduled, false
        };
        doses_.push_back(dose);
        return true;
    }

    // Add stock for a refill. Reject invalid cells, zero refills, and overflow.
    bool restockCell(uint8_t cellId, uint16_t units) {
        CellInventory* cell = findCell(cellId);
        if (cell == NULL || units == 0 ||
            units > static_cast<uint16_t>(UINT16_MAX - cell->stock)) {
            return false;
        }
        cell->stock = static_cast<uint16_t>(cell->stock + units);
        return true;
    }

    // Directly decrement stock by units. Clamps at zero.
    bool decrementStock(uint8_t cellId, uint16_t units = 1) {
        CellInventory* cell = findCell(cellId);
        if (cell == NULL || cell->stock == 0) {
            return false;
        }
        if (cell->stock < units) {
            cell->stock = 0;
        } else {
            cell->stock = static_cast<uint16_t>(cell->stock - units);
        }
        return true;
    }

    bool setCellStock(uint8_t cellId, uint16_t newStock) {
        CellInventory* cell = findCell(cellId);
        if (cell == NULL) {
            return false;
        }
        cell->stock = newStock;
        return true;
    }

    // Advance scheduled occurrences to Due or Missed based on RTC time.
    void update(uint64_t now) {
        for (size_t i = 0; i < doses_.size(); ++i) {
            DoseOccurrence& dose = doses_[i];
            if (dose.status != DoseStatus::Scheduled && dose.status != DoseStatus::Due) {
                continue;
            }
            if (now < dose.scheduledAt) {
                continue;
            }

            if (now - dose.scheduledAt >= missedGraceSeconds_) {
                dose.status = DoseStatus::Missed;
                dose.missedAlertPending = true;
                missedAlertPending_ = true;
            } else {
                dose.status = DoseStatus::Due;
            }
        }
    }

    // A cell button dismisses currently pending missed-dose notifications, but
    // does not change missed dose history or consume its stock. It then confirms
    // at most one active Due occurrence belonging to that same cell.
    DoseResult confirmCell(uint8_t cellId, uint64_t now) {
        update(now);
        acknowledgeMissedAlerts();

        if (findCell(cellId) == NULL) {
            return DoseResult::UnknownCell;
        }

        DoseOccurrence* nextDose = NULL;
        for (size_t i = 0; i < doses_.size(); ++i) {
            DoseOccurrence& dose = doses_[i];
            if (dose.cellId != cellId || dose.status != DoseStatus::Due) {
                continue;
            }
            if (nextDose == NULL || dose.scheduledAt < nextDose->scheduledAt) {
                nextDose = &dose;
            }
        }

        if (nextDose == NULL) {
            for (size_t i = 0; i < doses_.size(); ++i) {
                if (doses_[i].cellId == cellId && doses_[i].status == DoseStatus::Missed) {
                    return DoseResult::MissedDoseNotCounted;
                }
            }
            return DoseResult::NoDueDose;
        }

        CellInventory* cell = findCell(cellId);
        nextDose->status = DoseStatus::Confirmed;
        if (cell->stock < nextDose->units) {
            cell->stock = 0;
            return DoseResult::ConfirmedStockEmpty;
        }

        cell->stock = static_cast<uint16_t>(cell->stock - nextDose->units);
        return cell->isLow() ? DoseResult::ConfirmedLowStock : DoseResult::Confirmed;
    }

    // Clears the OLED/notification latch only. Missed statuses remain unchanged.
    void acknowledgeMissedAlerts() {
        missedAlertPending_ = false;
        for (size_t i = 0; i < doses_.size(); ++i) {
            doses_[i].missedAlertPending = false;
        }
    }

    bool hasMissedAlert() const {
        return missedAlertPending_;
    }

    size_t missedDoseCount() const {
        size_t count = 0;
        for (size_t i = 0; i < doses_.size(); ++i) {
            if (doses_[i].status == DoseStatus::Missed) {
                ++count;
            }
        }
        return count;
    }

    const CellInventory* getCell(uint8_t cellId) const {
        return findCell(cellId);
    }

    const DoseOccurrence* getDose(uint16_t doseId) const {
        return findDose(doseId);
    }

    size_t doseCount() const {
        return doses_.size();
    }

    size_t cellCount() const {
        return cells_.size();
    }

private:
    uint32_t missedGraceSeconds_;
    bool missedAlertPending_;
    std::vector<CellInventory> cells_;
    std::vector<DoseOccurrence> doses_;

    CellInventory* findCell(uint8_t cellId) {
        for (size_t i = 0; i < cells_.size(); ++i) {
            if (cells_[i].cellId == cellId) {
                return &cells_[i];
            }
        }
        return NULL;
    }

    const CellInventory* findCell(uint8_t cellId) const {
        for (size_t i = 0; i < cells_.size(); ++i) {
            if (cells_[i].cellId == cellId) {
                return &cells_[i];
            }
        }
        return NULL;
    }

    DoseOccurrence* findDose(uint16_t doseId) {
        for (size_t i = 0; i < doses_.size(); ++i) {
            if (doses_[i].doseId == doseId) {
                return &doses_[i];
            }
        }
        return NULL;
    }

    const DoseOccurrence* findDose(uint16_t doseId) const {
        for (size_t i = 0; i < doses_.size(); ++i) {
            if (doses_[i].doseId == doseId) {
                return &doses_[i];
            }
        }
        return NULL;
    }
};

#endif