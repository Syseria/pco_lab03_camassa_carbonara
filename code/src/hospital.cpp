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
    int ret = 0;

    mutex.lock();
    if(what == ItemType::PatientSick && stocks.at(what) >= qty) {
        currentBeds -= qty;
        stocks.at(what) -= qty;
        int bill = qty * getCostPerUnit(ItemType::PatientSick);
        money += bill;
        ret = bill;  
    }
    mutex.unlock();

    return ret;
}

void Hospital::freeHealedPatient() {

    mutex.lock();
    if(iterations >= 5) {
        if(stocks.at(ItemType::PatientHealed)){
            stocks.at(ItemType::PatientHealed)--;
            currentBeds--;
            nbFree++;
            iterations = 1;
        }
    } else {
        iterations++;
    }
    mutex.unlock();
}

void Hospital::transferPatientsFromClinic() {

    auto cl = chooseRandomSeller(clinics);
    int qty = 1;

    for(int i = 0; i < cl->getItemsForSale()[ItemType::PatientHealed]; i++) {

        mutex.lock();
        if(maxBeds >= (currentBeds + qty) && money >= qty * getEmployeeSalary(EmployeeType::Nurse)) {
            int bill = cl->request(ItemType::PatientHealed, 1);
            if(bill) {
                money -= bill + qty * getEmployeeSalary(EmployeeType::Nurse);
                stocks.at(ItemType::PatientHealed)++;
                currentBeds++;
            }
        } else {
            mutex.unlock();
            break;
        }
        mutex.unlock();
    }


}

int Hospital::send(ItemType it, int qty, int bill) {
    int newBill = bill + qty * getEmployeeSalary(EmployeeType::Nurse);

    int ret = 0;

    mutex.lock();
    if(money >= newBill && maxBeds >= (qty + currentBeds)) {
        money -= newBill;
        stocks.at(it) += qty;
        currentBeds += qty;
        nbHospitalised += qty;
        ret = bill;
    }
    mutex.unlock();

    return ret;
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
