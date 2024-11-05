#include <gtest/gtest.h>
#include "supplier.h"
#include "ambulance.h"
#include "iwindowinterface.h"
#include "fakeinterface.h"
#include <pcosynchro/pcothread.h>
#include <iostream>
#include <vector>
#include <random>
#include "utils.h"

void sendPatients(Hospital& hospital, ItemType itemType, std::atomic<int>& totalPaid) {
    int tot = 0;
    for (int i = 0; i < 20000; ++i) {
        int qty = 1;
        int bill = getCostPerUnit(itemType) * qty;
        if (hospital.send(itemType, qty, bill) > 0) { 
            tot += bill; 
        }
    }

    totalPaid += tot;
}

void requestPatients(Hospital& hospital, ItemType itemType, std::atomic<int>& totalGained) {
    int tot = 0;
    for (int i = 0; i < 20000; ++i) {
        int qty = 1;
        tot += hospital.request(itemType, qty);
    }

    totalGained += tot;
}

void requestSupply(Pharmacy& pharma, std::vector<ItemType> items, std::atomic<int>& totalGained) {
    int tot = 0;
    for (size_t i = 0; i < 20000; ++i) {
        int qty = 1;
        for (ItemType item : items) {
            tot += pharma.request(item, qty);
        }
    }

    totalGained += tot;
}

void requestMedicalSupply(MedicalDeviceSupplier& medicalDeviceSupplier, std::vector<ItemType> items, std::atomic<int>& totalGained) {
    int tot = 0;
    for (size_t i = 0; i < 20000; ++i) {
        int qty = 1;
        for (ItemType item : items) {
            tot += medicalDeviceSupplier.request(item, qty);
        }
    }

    totalGained += tot;
}

TEST(TestSuppliers, MedicalDeviceSupplierTest) {
    const int uniqueId = 0;
    const int initialFund = 20000;
    const unsigned int nbThreads = 4;
    int endFund = 0;
    std::atomic<int> totalPaid = 0;
    std::atomic<int> totalGained = 0;

    IWindowInterface* windowInterface = new FakeInterface();
    Supplier::setInterface(windowInterface);

    MedicalDeviceSupplier medicalDeviceSupplier(uniqueId, initialFund);

    std::vector<ItemType> items = { ItemType::Scalpel, ItemType::Thermometer, ItemType::Stethoscope };

    std::vector<std::unique_ptr<PcoThread>> threads;

    for (unsigned int i = 0; i < nbThreads; ++i) {
        threads.emplace_back(std::make_unique<PcoThread>(requestMedicalSupply, std::ref(medicalDeviceSupplier), items, std::ref(totalGained)));
    }

    for (auto& thread : threads) {
        thread->join();
    }

    endFund += medicalDeviceSupplier.getFund();
    endFund += medicalDeviceSupplier.getAmountPaidToWorkers();
    endFund += totalPaid;
    endFund -= totalGained;

    EXPECT_EQ(endFund, initialFund);
    EXPECT_GT(medicalDeviceSupplier.getQuantitySupplied(), 0);
}

TEST(TestSuppliers, PharmacyTest) {
    const int uniqueId = 0;
    const int initialFund = 20000;
    const unsigned int nbThreads = 4;
    int endFund = 0;
    std::atomic<int> totalPaid = 0;
    std::atomic<int> totalGained = 0;

    IWindowInterface* windowInterface = new FakeInterface();
    Supplier::setInterface(windowInterface);

    Pharmacy pharmacy(uniqueId, initialFund);

    std::vector<ItemType> items = { ItemType::Syringe, ItemType::Pill };

    std::vector<std::unique_ptr<PcoThread>> threads;

    for (unsigned int i = 0; i < nbThreads; ++i) {
        threads.emplace_back(std::make_unique<PcoThread>(requestSupply, std::ref(pharmacy), items, std::ref(totalGained)));
    }

    for (auto& thread : threads) {
        thread->join();
    }

    endFund += pharmacy.getFund();
    endFund += pharmacy.getAmountPaidToWorkers();
    endFund += totalPaid;
    endFund -= totalGained;

    EXPECT_EQ(endFund, initialFund);
    EXPECT_GT(pharmacy.getQuantitySupplied(), 0);
}

TEST(SellerTest, TestHospitals) {
    const int uniqueId = 0;
    const int initialFund = 20000;
    const unsigned int maxBeds = MAX_BEDS_PER_HOSTPITAL;
    const unsigned int nbThreads = 4;
    int endFund = 0;
    std::atomic<int> totalPaid = 0;
    std::atomic<int> totalGained = 0;

    IWindowInterface* windowInterface = new FakeInterface();
    Hospital::setInterface(windowInterface);

    Hospital hospital(uniqueId, initialFund, maxBeds);

    std::vector<std::unique_ptr<PcoThread>> threads;

    for (unsigned int i = 0; i < nbThreads / 2; ++i) {
        threads.emplace_back(std::make_unique<PcoThread>(requestPatients, std::ref(hospital), ItemType::PatientSick, std::ref(totalGained)));
        threads.emplace_back(std::make_unique<PcoThread>(sendPatients, std::ref(hospital), ItemType::PatientSick, std::ref(totalPaid))); 
    }

    for (auto& thread : threads) {
        thread->join();
    }

    endFund += hospital.getFund();
    endFund += hospital.getAmountPaidToWorkers();
    endFund += totalPaid; // <- En test cet argent est perdu
    endFund -= totalGained; // <- En test cette argent vient de nulle part

    EXPECT_EQ(endFund, initialFund);
    EXPECT_GE(hospital.getNumberPatients(), 0);
    EXPECT_LE(hospital.getNumberPatients(), maxBeds);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
