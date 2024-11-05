#include "clinic.h"
#include "costs.h"
#include <pcosynchro/pcothread.h>
#include <iostream>
#include <stdexcept>

IWindowInterface* Clinic::interface = nullptr;

Clinic::Clinic(int uniqueId, int fund, std::vector<ItemType> resourcesNeeded)
    : Seller(fund, uniqueId), nbTreated(0), resourcesNeeded(resourcesNeeded)
{
    interface->updateFund(uniqueId, fund);
    interface->consoleAppendText(uniqueId, "Factory created");

    for(const auto& item : resourcesNeeded) {
        mutex.lock();
        stocks[item] = 0;
        mutex.unlock();
    }
}

bool Clinic::verifyResources() {
    mutex.lock();
    for (auto item : resourcesNeeded) {
        if (stocks[item] <= 0) {
            mutex.unlock();
            return false;
        }
    }
    mutex.unlock();
    return true;
}

int Clinic::request(ItemType what, int qty){
    int price = 0;

    if (what != ItemType::PatientHealed) {
        return price;
    }

    mutex.lock();

    // If enough quantity in stocks and if qty is strictly greater than 0
    // we sell, else returns 0 at the end of function
    if (qty > 0 && stocks[what] >= qty) {
        price = getCostPerUnit(what) * qty;
        stocks[what] -= qty;
        money += price;
    }
    mutex.unlock();

    return price;
}

void Clinic::treatPatient() {
    int cost = getEmployeeSalary(getEmployeeThatProduces(ItemType::PatientHealed));
    bool canTreat = true;

    mutex.lock();
    if (!getWaitingPatients() && cost > money) {
        mutex.unlock();
        return;
    }

    for (ItemType item : resourcesNeeded) {
        if (stocks[item] <= 0) {
            canTreat = false;
        }
    }

    if (canTreat) {
        //Temps simulant un traitement
        interface->simulateWork();

        money -= cost;
        for (ItemType item : resourcesNeeded) {
            --stocks[item];
        }

        ++stocks[ItemType::PatientHealed];
        ++nbTreated;
    }

    mutex.unlock();
    interface->consoleAppendText(uniqueId, "Clinic have healed a new patient");
}

void Clinic::orderResources() {
    int qtyToBuy = 1;
    int cost = 0;

    for (ItemType item : resourcesNeeded) {
        switch (item) {
        case ItemType::PatientSick:
            for (auto hospital : hospitals) {
                mutex.lock();
                if ((cost = hospital->request(item, qtyToBuy)) < money && stocks[item] <= 0) {
                    money -= cost;
                    stocks[item] += qtyToBuy;
                    interface->consoleAppendText(uniqueId, "Clinic has gotten a new " + getItemName(item));
                }
                mutex.unlock();
            }
            break;
        default:
            for (auto supplier : suppliers) {
                mutex.lock();
                if (supplier->getItemsForSale().find(item) != getItemsForSale().end() && (cost = supplier->request(item, qtyToBuy)) < money && stocks[item] <= 0) {
                    money -= cost;
                    stocks[item] += qtyToBuy;
                    interface->consoleAppendText(uniqueId, "Clinic has bought a new " + getItemName(item));
                }
                mutex.unlock();
            }
            break;
        }
    }
}

void Clinic::run() {
    if (hospitals.empty() || suppliers.empty()) {
        std::cerr << "You have to give to hospitals and suppliers to run a clinic" << std::endl;
        return;
    }
    interface->consoleAppendText(uniqueId, "[START] Factory routine");

    while (!PcoThread::thisThread()->stopRequested()) {

        if (verifyResources()) {
            treatPatient();
        } else {
            orderResources();
        }
       
        interface->simulateWork();

        interface->updateFund(uniqueId, money);
        interface->updateStock(uniqueId, &stocks);
    }
    interface->consoleAppendText(uniqueId, "[STOP] Factory routine");
}


void Clinic::setHospitalsAndSuppliers(std::vector<Seller*> hospitals, std::vector<Seller*> suppliers) {
    this->hospitals = hospitals;
    this->suppliers = suppliers;

    for (Seller* hospital : hospitals) {
        interface->setLink(uniqueId, hospital->getUniqueId());
    }
    for (Seller* supplier : suppliers) {
        interface->setLink(uniqueId, supplier->getUniqueId());
    }
}

int Clinic::getTreatmentCost() {
    return 0;
}

int Clinic::getWaitingPatients() {
    return stocks[ItemType::PatientSick];
}

int Clinic::getNumberPatients(){
    return stocks[ItemType::PatientSick] + stocks[ItemType::PatientHealed];
}

int Clinic::send(ItemType it, int qty, int bill){
    return 0;
}

int Clinic::getAmountPaidToWorkers() {
    return nbTreated * getEmployeeSalary(getEmployeeThatProduces(ItemType::PatientHealed));
}

void Clinic::setInterface(IWindowInterface *windowInterface) {
    interface = windowInterface;
}

std::map<ItemType, int> Clinic::getItemsForSale() {
    return stocks;
}


Pulmonology::Pulmonology(int uniqueId, int fund) :
    Clinic::Clinic(uniqueId, fund, {ItemType::PatientSick, ItemType::Pill, ItemType::Thermometer}) {}

Cardiology::Cardiology(int uniqueId, int fund) :
    Clinic::Clinic(uniqueId, fund, {ItemType::PatientSick, ItemType::Syringe, ItemType::Stethoscope}) {}

Neurology::Neurology(int uniqueId, int fund) :
    Clinic::Clinic(uniqueId, fund, {ItemType::PatientSick, ItemType::Pill, ItemType::Scalpel}) {}
