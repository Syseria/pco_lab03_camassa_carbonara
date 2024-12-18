#include "supplier.h"
#include "costs.h"
#include <pcosynchro/pcothread.h>
#include <stdexcept>

IWindowInterface* Supplier::interface = nullptr;

Supplier::Supplier(int uniqueId, int fund, std::vector<ItemType> resourcesSupplied)
    : Seller(fund, uniqueId), resourcesSupplied(resourcesSupplied), nbSupplied(0) 
{
    for (const auto& item : resourcesSupplied) {    
        stocks[item] = 0;
    }

    interface->consoleAppendText(uniqueId, QString("Supplier Created"));
    interface->updateFund(uniqueId, fund);
}


int Supplier::request(ItemType it, int qty) {
    int price = 0;

    mutex.lock();
    auto listItem = getItemsForSale();

    // If enough quantity in stocks and if qty is strictly greater than 0
    // we sell, else returns 0 at the end of function
    if (qty > 0 && listItem.find(it) != listItem.end() && listItem[it] >= qty) {
        price = getCostPerUnit(it) * qty;
        stocks[it] -= qty;
        money += price;
        nbSupplied += qty;
    }
    mutex.unlock();

    return price;
}

void Supplier::run() {
    interface->consoleAppendText(uniqueId, "[START] Supplier routine");
    while (!PcoThread::thisThread()->stopRequested()) {
        ItemType resourceSupplied = getRandomItemFromStock();
        int supplierCost = getEmployeeSalary(getEmployeeThatProduces(resourceSupplied));

        mutex.lock();
        if (money < supplierCost) {
            continue;
        }

        /* Temps aléatoire borné qui simule l'attente du travail fini*/
        interface->simulateWork();

        money -= supplierCost;
        stocks.at(resourceSupplied) += 1;
        mutex.unlock();

        interface->updateFund(uniqueId, money);
        interface->updateStock(uniqueId, &stocks);
    }
    interface->consoleAppendText(uniqueId, "[STOP] Supplier routine");
}


std::map<ItemType, int> Supplier::getItemsForSale() {
    return stocks;
}

int Supplier::getMaterialCost() {
    int totalCost = 0;
    for (const auto& item : resourcesSupplied) {
        totalCost += getCostPerUnit(item);
    }
    return totalCost;
}

int Supplier::getAmountPaidToWorkers() {
    return nbSupplied * getEmployeeSalary(EmployeeType::Supplier);
}

void Supplier::setInterface(IWindowInterface *windowInterface) {
    interface = windowInterface;
}

std::vector<ItemType> Supplier::getResourcesSupplied() const
{
    return resourcesSupplied;
}


int Supplier::getQuantitySupplied() const {
    return nbSupplied;
}

int Supplier::send(ItemType it, int qty, int bill){
    return 0;
}
