#include <QtTest>
#include "core/orientation_generator.h"

class TestOrientationGenerator : public QObject {
    Q_OBJECT

private slots:
    void testGenerate();
    void testSingularPoints();
};

void TestOrientationGenerator::testGenerate() {
    SFinGe::OrientationGenerator generator;
    SFinGe::SingularPoints points;
    points.addCore(50, 50);
    points.addDelta(80, 120);
    
    std::vector<float> shapeMap(100 * 100, 1.0f);
    
    generator.setSingularPoints(points);
    generator.setShapeMap(shapeMap, 100, 100);
    
    QImage image = generator.generateVisualization();
    
    QVERIFY(!image.isNull());
    QCOMPARE(image.width(), 100);
    QCOMPARE(image.height(), 100);
}

void TestOrientationGenerator::testSingularPoints() {
    SFinGe::SingularPoints points;
    points.addCore(10, 20);
    points.addDelta(30, 40);
    
    QCOMPARE(points.getCoreCount(), 1);
    QCOMPARE(points.getDeltaCount(), 1);
    
    SFinGe::SingularPoint core = points.getCore(0);
    QCOMPARE(core.x, 10.0);
    QCOMPARE(core.y, 20.0);
}

QTEST_MAIN(TestOrientationGenerator)
#include "test_orientation_generator.moc"
