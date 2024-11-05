#include "hospital.h"
#include "costs.h"
#include <iostream>
#include <pcosynchro/pcothread.h>

IWindowInterface* Hospital::interface = nullptr;

Hospital::Hospital(int uniqueId, int fund, int maxBeds)
    : Seller(fund, uniqueId), maxBeds(maxBeds), currentBeds(0), nbHospitalised(0), nbFree(0)
{
    interface->updateFund(uniqueId, fund);
    interface->consoleAppendText(uniqueId, "Hospital Created with " + QString::number(maxBeds) + " beds");
    
    std::vector<ItemType> initialStocks = { ItemType::PatientHealed, ItemType::PatientSick };

    for(const auto& item : initialStocks) {
        stocks[item] = 0;
    }
}

int Hospital::request(ItemType what, int qty){

    if(what == ItemType::PatientSick && this->stocks.at(what) >= qty) {
        mutex.lock();
        currentBeds -= qty;
        stocks.at(what) -= qty;
        int bill = qty * getCostPerUnit(ItemType::PatientSick);
        money += bill;
        mutex.unlock();
        return qty;
    }

    return 0;
}

void Hospital::freeHealedPatient() {
    if(stocks.at(ItemType::PatientHealed)){
        mutex.lock();
        stocks.at(ItemType::PatientHealed)--;
        currentBeds--;
        nbFree++;
        mutex.unlock();
    }
}

void Hospital::transferPatientsFromClinic() {

    auto cl = chooseRandomSeller(clinics);
    int qty = 1;

    for(int i = 0; i < cl->getItemsForSale().at(ItemType::PatientHealed); i++) {
        if(maxBeds >= (currentBeds + qty) && money >= getEmployeeSalary(EmployeeType::Nurse)) {
            int patients = cl->request(ItemType::PatientHealed, 1);
            if(patients) {
                mutex.lock();
                money -= getEmployeeSalary(EmployeeType::Nurse);
                stocks.at(ItemType::PatientHealed)++;
                currentBeds++;
                mutex.unlock();
            }
        } else {
            break;
        }
    }

}

int Hospital::send(ItemType it, int qty, int bill) {
    int newBill = bill + getEmployeeSalary(EmployeeType::Nurse);
    if(money >= newBill && maxBeds >= (qty + currentBeds)) {
        mutex.lock();
        money -= newBill;
        stocks.at(it) += qty;
        currentBeds += qty;
        nbHospitalised += qty;
        mutex.unlock();
        return bill;
    }
    return 0;
}

void Hospital::run()
{
    if (clinics.empty()) {
        std::cerr << "You have to give clinics to a hospital before launching is routine" << std::endl;
        return;
    }

    interface->consoleAppendText(uniqueId, "[START] Hospital routine");

    while (!PcoThread::thisThread()->stopRequested()) {
        transferPatientsFromClinic();

        freeHealedPatient();

        interface->updateFund(uniqueId, money);
        interface->updateStock(uniqueId, &stocks);
        interface->simulateWork(); // Temps d'attente
    }

    interface->consoleAppendText(uniqueId, "[STOP] Hospital routine");
}

int Hospital::getAmountPaidToWorkers() {
    return nbHospitalised * getEmployeeSalary(EmployeeType::Nurse);
}

int Hospital::getNumberPatients(){
    return stocks[ItemType::PatientSick] + stocks[ItemType::PatientHealed] + nbFree;
}

std::map<ItemType, int> Hospital::getItemsForSale()
{
    return stocks;
}

void Hospital::setClinics(std::vector<Seller*> clinics){
    this->clinics = clinics;

    for (Seller* clinic : clinics) {
        interface->setLink(uniqueId, clinic->getUniqueId());
    }
}

void Hospital::setInterface(IWindowInterface* windowInterface){
    interface = windowInterface;
}
