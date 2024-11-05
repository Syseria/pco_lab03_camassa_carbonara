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
        stocks[item] = 0;
    }
}

bool Clinic::verifyResources() {
    for (auto item : resourcesNeeded) {
        if (stocks[item] == 0) {
            return false;
        }
    }
    return true;
}

int Clinic::request(ItemType what, int qty){
    int price = 0;

    mutex.lock();
    std::map<ItemType, int> listItem = getItemsForSale();

    // try catch due to map.at() throwing out_of_range exception if key is not found
    // correspond to the case "Objet non vendu"
    try {
        int& item = listItem.at(what);

        // If enough quantity in stocks and if qty is strictly greater than 0
        // we sell, else returns 0 at the end of function
        if (qty > 0 && item >= qty) {
            price = getCostPerUnit(what) * qty;
            item -= qty;
            money += price;
        }
        mutex.unlock();
    } catch (const std::out_of_range& e) {
        mutex.unlock();
        return price;
    }

    return price;
}

void Clinic::treatPatient() {
    int cost = getEmployeeSalary(getEmployeeThatProduces(ItemType::PatientHealed));

    if (!getWaitingPatients() && cost > money) {
        return;
    }

    for (ItemType item : resourcesNeeded) {
        if (item == ItemType::PatientSick) {
            continue;
        }
        mutex.lock();
        --stocks.at(item);
        mutex.unlock();
    }

    //Temps simulant un traitement
    interface->simulateWork();

    mutex.lock();
    money -= cost;
    --stocks.at(ItemType::PatientSick);
    ++nbTreated;
    mutex.unlock();
    
    interface->consoleAppendText(uniqueId, "Clinic have healed a new patient");
}

void Clinic::orderResources() {
    int qtyToBuy = 1;
    int cost = 0;

    mutex.lock();
    for (ItemType item : resourcesNeeded) {
        if (item == ItemType::PatientSick && stocks.at(item) <= 0) {
            for (auto hospital : hospitals) {
                if (getCostPerUnit(item) * qtyToBuy < money) {
                    cost += hospital->request(item, qtyToBuy);
                }
            }
        } else {
            for (auto supplier : suppliers) {
                if (getCostPerUnit(item) * qtyToBuy < money) {
                    cost += supplier->request(item, qtyToBuy);
                }
            }
        }

        money -= cost;
        stocks.at(item) += qtyToBuy;
    }
    mutex.unlock();
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
