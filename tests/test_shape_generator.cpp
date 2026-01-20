#include <QtTest>
#include "core/shape_generator.h"

class TestShapeGenerator : public QObject {
    Q_OBJECT

private slots:
    void testGenerate();
    void testParameters();
};

void TestShapeGenerator::testGenerate() {
    SFinGe::ShapeGenerator generator;
    SFinGe::ShapeParameters params;
    params.left = 50;
    params.right = 50;
    params.top = 50;
    params.bottom = 50;
    params.middle = 50;
    
    generator.setParameters(params);
    QImage image = generator.generate();
    
    QVERIFY(!image.isNull());
    QCOMPARE(image.width(), 100);
    QCOMPARE(image.height(), 150);
}

void TestShapeGenerator::testParameters() {
    SFinGe::ShapeGenerator generator;
    SFinGe::ShapeParameters params;
    params.left = 30;
    params.right = 40;
    
    generator.setParameters(params);
    QCOMPARE(generator.getWidth(), 70);
}

QTEST_MAIN(TestShapeGenerator)
#include "test_shape_generator.moc"
