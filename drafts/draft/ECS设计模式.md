# ECS设计模式

> 太长不看版： ECS就是Entity/Component/System的首字母缩写。
> 以面向对象编程的思路来类比，把原先叫Class的，改成叫Entity, 原先Class里面的数据成员，打散后按某种规则分成组，每组里面有一个或多个成员，这个组就叫Component，把原先Class里面的函数，打散后改成叫某种System，这个System只根据某几种Component进行操作。
> 所以，原先OOP中，Class/Member/Function之间的关系，会变为：某种Entity对应多种Component，某种System根据多种Component的数据进行操作的关系

## 什么是ECS

ECS是基于DOD思想下的一种设计模式，其实质是一种对事物如何抽象的思路，与一般的OOP思路不同，ECS是把数据和行为区分开，数据按照某种规则关联在一起，称为Component(组件)，多个组件可以描述某种事物，称为Entity(实体)，而对这些实体的某几个Component进行操作，可归纳为某种System(系统)。

ECS的诞生是要解决复杂和多变场景下，事物的描述和操作和管理上较为繁琐的问题，其次是性能问题，归纳如下:

ECS的使用场景：

1. 事物的描述比较复杂，比如事物具有很多的属性，而且事物的种类繁多(也就是Class的数据成员多，Class之间的横向，纵向关系复杂)
2. 事物描述经常性的变化(也就是Class的声明/定义经常修改)
3. 在1，2的前提下，对性能要求比较高

此模式一般多用于游戏设计中，因为一个稍微复杂的游戏，一般就满足以上三点

最简例子如下:

PlayerEntity(PositionComponent, AttrComponent)
PlayerEntity代表玩家这种事物，其包含位置，属性的相关数据

其中MoveComponent包含一组坐标数据
MoveComponent(x,y,z)

AttrComponent也包含血量，魔量数据
AttrComponent(hp,mp)

现在有一个MoveSystem,主要是对含有PositionComponent的所有Entity更新位置信息,进行移动，伪代码如下:

for(每个PositionComponent : 所有PositionComponent) {
    每个PositionComponent.x += 1;
    每个PositionComponent.y += 1;
    每个PositionComponent.z += 1;
}

如果是基于OOP的思路，那么伪代码类似下面的例子:

for(每个对象 : 所有实现了Move接口的对象) {
    每个对象.move(1,1,1);
}

可看到，ECS(或者说DOP/DOD)不一定在代码设计上优于OOP，最大的区别是，由于逻辑和数据的分离，更方便对相同类型的数据进行紧凑安排，以便提高缓存命中率和内存局部性，进而提高性能，其次只在很复杂和多变的事物描述上，维护起来更方便，只用加对应的E/C/S就行，还可以复用之前的E/C/S, 它们之间不像OOP，类，成员，函数一般来说是强绑定的

那么Component，Entity,System 就可以根据需要进行自由组装，这样做有利于对数据做紧凑排布，以便提升缓存命中，进而提升性能；同时，未来新增新的数据和行为时，也不会导致系统很多地方也需要更改，可减少维护和开发成本。

## ECS的实现

实现思路上，主要是注意几个点:

1. 不同Entity, Component, System的区分
2. E/C/S之间如何进行关联
3. 如何管理Component里面的数据，使其尽量连续和紧凑
