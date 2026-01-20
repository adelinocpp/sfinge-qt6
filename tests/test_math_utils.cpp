#include <QtTest>
#include "core/math_utils.h"

class TestMathUtils : public QObject {
    Q_OBJECT

private slots:
    void testInsideEllipse();
    void testNoise();
    void testRenderClouds();
};

void TestMathUtils::testInsideEllipse() {
    QVERIFY(SFinGe::insideEllipse(50, 50, 30, 30, 50, 50));
    QVERIFY(SFinGe::insideEllipse(50, 50, 30, 30, 60, 60));
    QVERIFY(!SFinGe::insideEllipse(50, 50, 30, 30, 100, 100));
}

void TestMathUtils::testNoise() {
    double n1 = SFinGe::noise(0, 0);
    double n2 = SFinGe::noise(1, 1);
    QVERIFY(n1 >= -1.0 && n1 <= 1.0);
    QVERIFY(n2 >= -1.0 && n2 <= 1.0);
}

void TestMathUtils::testRenderClouds() {
    auto clouds = SFinGe::renderClouds(100, 100, 1.0, 0.5);
    QCOMPARE(clouds.size(), 10000);
    
    for (float value : clouds) {
        QVERIFY(value >= 0.0f && value <= 1.0f);
    }
}

QTEST_MAIN(TestMathUtils)
#include "test_math_utils.moc"
