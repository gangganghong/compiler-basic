func test(age ,height float64) (int,float32){
  var res int = age
  var hei float32 = height

	select {
case i1 = <-c1:
	print("received ", i1, " from c1\n")
case c2 <- i2:
	print("sent ", i2, " to c2\n")
case i3, ok := (<-c3):  
	if ok {
		print("received ", i3, " from c3\n")
	} else {
		print("c3 is closed\n")
	}
case a[f()] = <-c4:
	
	
	
default:
	print("no communication\n")
}
}
